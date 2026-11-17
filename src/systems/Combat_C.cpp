#include "systems/Combat_C.h"

#include "core/World_U.h"
#include "data/EnemyDef_C.h"
#include "data/PlayerDef_C.h"
#include "systems/SpatialQuery_C.h"

#include <algorithm>

namespace
{
// 몸 앞쪽(iFacing 방향)에 붙은 사각형, 세로는 몸 가운데 맞춤
Rect ComputeHitbox(const Body& tBody, int iFacing)
{
    const Vec2 vSize = kPlayerDef.vAttackSize;
    const float fX = iFacing > 0 ? tBody.vPos.fX + tBody.vSize.fX : tBody.vPos.fX - vSize.fX;
    const float fY = tBody.vPos.fY + (tBody.vSize.fY - vSize.fY) * 0.5f;
    return Rect{fX, fY, vSize.fX, vSize.fY};
}

// 누가 누구를 때릴 수 있나. PvP 때 iTeam 비교로 교체 (hp-and-hit.md 결정 4)
bool IsHostile(const Entity& tAttacker, const Entity& tTarget)
{
    return (tAttacker.eType == EntityType::Player && tTarget.eType == EntityType::Enemy) ||
           (tAttacker.eType == EntityType::Enemy && tTarget.eType == EntityType::Player);
}

float GetAttackDamage(const Entity& tAttacker)  // TODO(Stats): 최종 공격력
{
    return tAttacker.eType == EntityType::Player ? kPlayerDef.fAttackDamage : kChaserDef.fAttackDamage;
}

bool WasAlreadyHit(const AttackState& tAttack, Handle hTarget)
{
    const auto itBegin = tAttack.arrAlreadyHit.begin();
    const auto itEnd = itBegin + tAttack.iAlreadyHitCount;
    return std::find(itBegin, itEnd, hTarget) != itEnd;
}

Vec2 OverlapCenter(const Rect& rcA, const Rect& rcB)
{
    const float fLeft = std::max(rcA.fX, rcB.fX);
    const float fRight = std::min(rcA.fX + rcA.fW, rcB.fX + rcB.fW);
    const float fTop = std::max(rcA.fY, rcB.fY);
    const float fBottom = std::min(rcA.fY + rcA.fH, rcB.fY + rcB.fH);
    return Vec2{(fLeft + fRight) * 0.5f, (fTop + fBottom) * 0.5f};
}
} // namespace

void Combat::UpdatePlayerAttack(const AttackIntent& tIntent, const Body& tBody, int iFacing, AttackState& tAttack,
                                float fDt) const
{
    tAttack.fBufferTimer =
        tIntent.bAttackPressed ? kPlayerDef.fAttackBufferTime : std::max(0.0f, tAttack.fBufferTimer - fDt);

    // 단계 진행
    if (tAttack.ePhase == AttackPhase::Active)
    {
        tAttack.fTimer -= fDt;
        if (tAttack.fTimer <= 0.0f)
        {
            tAttack.ePhase = AttackPhase::Recover;
            tAttack.fTimer = kPlayerDef.fAttackRecoverTime;
        }
    }
    else if (tAttack.ePhase == AttackPhase::Recover)
    {
        tAttack.fTimer -= fDt;
        if (tAttack.fTimer <= 0.0f)
            tAttack.ePhase = AttackPhase::Ready;
    }

    // 시작: Recover가 끝난 같은 틱에도 버퍼가 남아 있으면 바로 시작
    if (tAttack.ePhase == AttackPhase::Ready && tAttack.fBufferTimer > 0.0f)
    {
        tAttack.ePhase = AttackPhase::Active;
        tAttack.fTimer = kPlayerDef.fAttackActiveTime;
        tAttack.fBufferTimer = 0.0f;
        tAttack.iAlreadyHitCount = 0;  // 새 휘두름 -> 다시 맞힐 수 있음
    }

    if (tAttack.ePhase == AttackPhase::Active)
        tAttack.rcHitbox = ComputeHitbox(tBody, iFacing);  // 매 틱 다시 계산 -> 몸을 따라감
}

void Combat::ResolveHits(World& tWorld, const SpatialQuery& tSpatial, std::vector<HitReport>& vecOutReports,
                         float fDt)
{
    tWorld.ForEachAlive([&](Handle, Entity& tEntity) {
        tEntity.tHit.fFlashTimer = std::max(0.0f, tEntity.tHit.fFlashTimer - fDt);
    });

    tWorld.ForEachAlive([&](Handle hAttacker, Entity& tAttacker) {
        AttackState& tAttack = tAttacker.tAttack;
        if (tAttack.ePhase != AttackPhase::Active)
            return;

        tSpatial.QueryNearby(tAttack.rcHitbox, m_vecQueryBuffer);
        for (const Handle hTarget : m_vecQueryBuffer)
        {
            // 값 수정만 하고 칸 추가·삭제는 안 함 -> 순회 중 Get으로 다른 엔티티를 고쳐도 안전
            Entity* pTarget = tWorld.Get(hTarget);
            if (pTarget == nullptr || hTarget == hAttacker || !IsHostile(tAttacker, *pTarget))
                continue;
            if (pTarget->fHp <= 0.0f)
                continue;  // 이번 틱에 이미 죽음 (삭제 예약 중에도 Get은 유효, world.md 4장)
            if (WasAlreadyHit(tAttack, hTarget) || tAttack.iAlreadyHitCount >= kMaxHitsPerSwing)
                continue;

            tAttack.arrAlreadyHit[static_cast<std::size_t>(tAttack.iAlreadyHitCount)] = hTarget;
            ++tAttack.iAlreadyHitCount;

            const int iDir = tAttacker.tMotor.iFacing;
            pTarget->fHp -= GetAttackDamage(tAttacker);
            pTarget->tBody.vVel = {static_cast<float>(iDir) * kPlayerDef.fKnockbackSpeed, -kPlayerDef.fKnockbackUp};
            pTarget->tHit.fFlashTimer = kPlayerDef.fHitFlashTime;
            vecOutReports.push_back({.vPos = OverlapCenter(tAttack.rcHitbox, GetBounds(pTarget->tBody)),
                                     .iDir = iDir,
                                     .bKilled = pTarget->fHp <= 0.0f});

            // 플레이어는 HP 0이어도 삭제 안 함 -> 게임 오버는 RoomFlow (결정 3)
            if (pTarget->fHp <= 0.0f && pTarget->eType != EntityType::Player)
                tWorld.Destroy(hTarget);
        }
    });
}
