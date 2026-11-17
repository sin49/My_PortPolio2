#pragma once

#include "core/Body_C.h"
#include "core/MoveIntent_C.h"
#include "systems/Combat_C.h"
#include "systems/PlayerMotor_C.h"

enum class EntityType
{
    Player,
    Enemy,
    Drop,
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
    // 종류별 상태(PlayerMotorState, EnemyState 등)는 필요해질 때 추가
};
