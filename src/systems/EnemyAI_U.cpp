#include "systems/EnemyAI_U.h"

#include "core/World_U.h"
#include "data/EnemyDef_C.h"

#include <cmath>  // std::abs

namespace
{
void Enter(EnemyAIState& tAI, EnemyState eNext)
{
    tAI.eState = eNext;
    if (eNext == EnemyState::Windup)
        tAI.fTimer = kChaserDef.fWindupTime;
    else if (eNext == EnemyState::Recover)
        tAI.fTimer = kChaserDef.fRecoverTime;
    else
        tAI.fTimer = 0.0f;
}

float CenterX(const Body& tBody)
{
    return tBody.vPos.fX + tBody.vSize.fX * 0.5f;
}

float CenterY(const Body& tBody)
{
    return tBody.vPos.fY + tBody.vSize.fY * 0.5f;
}

void UpdateEnemy(Entity& tEnemy, const Entity* pPlayer, float fDt)
{
    EnemyAIState& tAI = tEnemy.tAI;
    tAI.fTimer -= fDt;

    if (pPlayer == nullptr)
    {
        if (tAI.eState != EnemyState::Idle)
            Enter(tAI, EnemyState::Idle);
        return;
    }

    const float fDx = CenterX(pPlayer->tBody) - CenterX(tEnemy.tBody);
    const float fDy = std::abs(CenterY(pPlayer->tBody) - CenterY(tEnemy.tBody));
    const float fGapX = std::abs(fDx) - (pPlayer->tBody.vSize.fX + tEnemy.tBody.vSize.fX) * 0.5f;

    switch (tAI.eState)
    {
    case EnemyState::Idle:
        if (std::abs(fDx) < kChaserDef.fDetectRange && fDy < kChaserDef.fDetectRange)
            Enter(tAI, EnemyState::Chase);
        break;

    case EnemyState::Chase:
        if (std::abs(fDx) > kChaserDef.fLoseRange)
        {
            Enter(tAI, EnemyState::Idle);
            break;
        }

        tEnemy.tMoveIntent.iMoveDir = (fDx > 0.0f) ? 1 : -1;

        if (fGapX <= kChaserDef.fAttackRangeX && fDy <= kChaserDef.fAttackRangeY)
            Enter(tAI, EnemyState::Windup);
        break;

    case EnemyState::Windup:
        if (tAI.fTimer <= 0.0f)
            Enter(tAI, EnemyState::Attack);
        break;

    case EnemyState::Attack:
        tEnemy.tAttackIntent.bAttackPressed = true;
        Enter(tAI, EnemyState::Recover);
        break;

    case EnemyState::Recover:
        if (tAI.fTimer <= 0.0f)
            Enter(tAI, EnemyState::Chase);
        break;
    }
}
} // namespace

void EnemyAI::Update(World& tWorld, Handle hPlayer, float fDt) const
{
    const Entity* pPlayer = tWorld.Get(hPlayer);

    tWorld.ForEachAlive([&](Handle, Entity& tEntity) {
        if (tEntity.eType != EntityType::Enemy)
            return;
        if (tEntity.fHp <= 0.0f)
            return;
        if (tEntity.tAI.bPassive)
            return;
        tEntity.tMoveIntent = {};
        tEntity.tAttackIntent = {};
        UpdateEnemy(tEntity, pPlayer, fDt);
    });
}
