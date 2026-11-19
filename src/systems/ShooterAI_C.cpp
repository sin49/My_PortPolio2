#include "systems/ShooterAI_C.h"

#include "core/World_U.h"
#include "data/EnemyDef_C.h"

#include <cmath>

namespace
{
void Enter(EnemyAIState& tAI, EnemyState eNext)
{
    tAI.eState = eNext;
    if (eNext == EnemyState::Windup)
        tAI.fTimer = kShooterDef.fWindupTime;
    else if (eNext == EnemyState::Recover)
        tAI.fTimer = kShooterDef.fRecoverTime;
    else
        tAI.fTimer = 0.0f;
}

Vec2 CenterOf(const Body& tBody)
{
    return Vec2{tBody.vPos.fX + tBody.vSize.fX * 0.5f, tBody.vPos.fY + tBody.vSize.fY * 0.5f};
}

// 가로 거리를 [fPreferMin, fPreferMax] 안으로: 가까우면 반대로, 멀면 다가감, 안이면 0
int KeepDistanceDir(float fDx)
{
    const float fDist = std::abs(fDx);
    const int iToward = fDx > 0.0f ? 1 : -1;
    if (fDist < kShooterDef.fPreferMin)
        return -iToward;
    if (fDist > kShooterDef.fPreferMax)
        return iToward;
    return 0;
}

void UpdateShooter(Entity& tShooter, const Entity* pPlayer, float fDt)
{
    EnemyAIState& tAI = tShooter.tAI;
    tAI.fTimer -= fDt;

    if (pPlayer == nullptr)
    {
        if (tAI.eState != EnemyState::Idle)
            Enter(tAI, EnemyState::Idle);
        return;
    }

    const Vec2 vPlayer = CenterOf(pPlayer->tBody);
    const Vec2 vSelf = CenterOf(tShooter.tBody);
    const float fDx = vPlayer.fX - vSelf.fX;
    const float fDy = std::abs(vPlayer.fY - vSelf.fY);
    const bool bInFireRange = std::abs(fDx) <= kShooterDef.fPreferMax && fDy <= kShooterDef.fFireRangeY;

    switch (tAI.eState)
    {
    case EnemyState::Idle:
        if (std::abs(fDx) < kShooterDef.fDetectRange && fDy < kShooterDef.fDetectRange)
            Enter(tAI, EnemyState::Chase);
        break;

    case EnemyState::Chase:  // 거리 맞추기
        if (std::abs(fDx) > kShooterDef.fLoseRange)
        {
            Enter(tAI, EnemyState::Idle);
            break;
        }
        tShooter.tMoveIntent.iMoveDir = KeepDistanceDir(fDx);
        if (bInFireRange)
            Enter(tAI, EnemyState::Windup);
        break;

    case EnemyState::Windup:  // 멈춰서 예고
        if (tAI.fTimer <= 0.0f)
            Enter(tAI, EnemyState::Attack);
        break;

    case EnemyState::Attack:  // 딱 1틱: 플레이어 쪽으로 발사 의도
        tShooter.tAttackIntent.bFirePressed = true;
        tShooter.tAttackIntent.vAim = {fDx, vPlayer.fY - vSelf.fY};
        tShooter.tAttackIntent.iShotCount = 1;
        Enter(tAI, EnemyState::Recover);
        break;

    case EnemyState::Recover:  // 쉬면서도 거리는 맞춤
        tShooter.tMoveIntent.iMoveDir = KeepDistanceDir(fDx);
        if (tAI.fTimer <= 0.0f)
            Enter(tAI, EnemyState::Chase);
        break;
    }
}
} // namespace

void ShooterAI::Update(World& tWorld, Handle hPlayer, float fDt) const
{
    const Entity* pPlayer = tWorld.Get(hPlayer);
    tWorld.ForEachAlive([&](Handle, Entity& tEntity) {
        if (tEntity.eType != EntityType::Shooter || tEntity.fHp <= 0.0f || tEntity.tAI.bPassive)
            return;
        tEntity.tMoveIntent = {};
        tEntity.tAttackIntent = {};
        UpdateShooter(tEntity, pPlayer, fDt);
    });
}
