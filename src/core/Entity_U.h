#pragma once

#include "core/Body_C.h"
#include "core/MoveIntent_C.h"
#include "systems/Combat_C.h"
#include "systems/PlayerMotor_C.h"
#include "systems/EnemyAI_U.h"
#include "systems/Projectile_C.h"
#include "systems/BossAI_C.h"

enum class EntityType
{
    Player,
    Enemy,
    Drop,
    Boss,
    Projectile,
    Shooter
};

struct Entity
{
    EntityType eType = EntityType::Enemy;
    Body tBody;
    float fHp = 0.0f;
    MoveIntent tMoveIntent;
    AttackIntent tAttackIntent;
    PlayerMotorState tMotor;
    AttackState tAttack;
    HitState tHit;
    EnemyAIState tAI;
    ProjectileState tProjectile;  // 투사체만 씀
    BossAIState tBoss;            // 보스만 씀 (11/19: 사용자 "알아서" 결정으로 AI가 2줄 추가)
    // 종류별 상태(PlayerMotorState, EnemyState 등)는 필요해질 때 추가
};
