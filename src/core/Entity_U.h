#pragma once

#include "core/Body_C.h"

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
    // 종류별 상태(PlayerMotorState, EnemyState 등)는 필요해질 때 추가
};
