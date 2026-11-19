#include "systems/BossAI_C.h"

#include "core/World_U.h"
#include "data/EnemyDef_C.h"

#include <cmath>

namespace
{
bool IsPhase2(const Entity& tBoss)
{
    return tBoss.fHp <= kBossDef.fMaxHp * kBossDef.fPhase2HpRatio;
}

// 상태 들어갈 때 타이머를 데이터 테이블에서 (사용자 EnemyAI의 Enter와 같은 모양)
void Enter(BossAIState& tAI, BossState eNext, bool bPhase2)
{
    tAI.eState = eNext;
    switch (eNext)
    {
    case BossState::AimedWindup:
        tAI.fTimer = kBossDef.fAimedWindupTime;
        break;
    case BossState::RingWindup:
        tAI.fTimer = kBossDef.fRingWindupTime;
        break;
    case BossState::Recover:
        tAI.fTimer = bPhase2 ? kBossDef.fRecoverTimePhase2 : kBossDef.fRecoverTime;
        break;
    default:
        tAI.fTimer = 0.0f;
        break;
    }
}

Vec2 CenterOf(const Body& tBody)
{
    return Vec2{tBody.vPos.fX + tBody.vSize.fX * 0.5f, tBody.vPos.fY + tBody.vSize.fY * 0.5f};
}

void UpdateBoss(Entity& tBoss, const Entity* pPlayer, float fDt)
{
    BossAIState& tAI = tBoss.tBoss;
    tAI.fTimer -= fDt;
    if (pPlayer == nullptr)
        return;  // 플레이어가 없으면 그 자리에서 대기 (게임 오버 직후 등)

    const bool bPhase2 = IsPhase2(tBoss);
    const Vec2 vPlayer = CenterOf(pPlayer->tBody);
    const Vec2 vSelf = CenterOf(tBoss.tBody);
    const float fDx = vPlayer.fX - vSelf.fX;

    switch (tAI.eState)
    {
    case BossState::Idle:
        if (std::abs(fDx) < kBossDef.fDetectRange)
            Enter(tAI, BossState::AimedWindup, bPhase2);
        break;

    case BossState::AimedWindup:  // 멈춰서 예고 (Render가 깜빡임)
        if (tAI.fTimer <= 0.0f)
            Enter(tAI, BossState::AimedFire, bPhase2);
        break;

    case BossState::AimedFire:  // 딱 1틱: 플레이어 쪽 부채꼴 3발
        tBoss.tAttackIntent = {.bFirePressed = true,
                               .vAim = {fDx, vPlayer.fY - vSelf.fY},
                               .iShotCount = kBossDef.iAimedShots,
                               .fSpreadDeg = kBossDef.fAimedSpreadDeg};
        tAI.bNextIsRing = true;
        Enter(tAI, BossState::Recover, bPhase2);
        break;

    case BossState::RingWindup:
        if (tAI.fTimer <= 0.0f)
            Enter(tAI, BossState::RingFire, bPhase2);
        break;

    case BossState::RingFire:  // 딱 1틱: 8방향 (fSpreadDeg 360 = 원형으로 고르게)
        tBoss.tAttackIntent = {.bFirePressed = true,
                               .vAim = {1.0f, 0.0f},
                               .iShotCount = kBossDef.iRingShots,
                               .fSpreadDeg = 360.0f};
        tAI.bNextIsRing = false;
        Enter(tAI, BossState::Recover, bPhase2);
        break;

    case BossState::Recover:  // 플레이어 쪽으로 천천히 걸음 -> 다음 패턴 예고
        tBoss.tMoveIntent.iMoveDir = fDx > 0.0f ? 1 : -1;
        if (tAI.fTimer <= 0.0f)
            Enter(tAI, tAI.bNextIsRing ? BossState::RingWindup : BossState::AimedWindup, bPhase2);
        break;
    }
}
} // namespace

void BossAI::Update(World& tWorld, Handle hPlayer, float fDt) const
{
    const Entity* pPlayer = tWorld.Get(hPlayer);
    tWorld.ForEachAlive([&](Handle, Entity& tEntity) {
        if (tEntity.eType != EntityType::Boss || tEntity.fHp <= 0.0f)
            return;
        tEntity.tMoveIntent = {};
        tEntity.tAttackIntent = {};
        UpdateBoss(tEntity, pPlayer, fDt);
    });
}
