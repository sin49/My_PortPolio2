#pragma once

#include "core/Handle_U.h"

// 적 AI (명시적 FSM). 의도(tMoveIntent·tAttackIntent)만 씀. 설계: docs/design/chaser.md
enum class EnemyState
{
    Idle,
    Chase,
    Windup,
    Attack,
    Recover,
};

struct EnemyAIState
{
    EnemyState eState = EnemyState::Idle;
    float fTimer = 0.0f;     // 현재 상태 남은 시간 (Windup·Recover만 씀)
    bool bPassive = false;   // true = 튜토리얼 허수아비: AI가 아무것도 안 함
};

class World;  // 전방 선언: Entity_U.h -> EnemyAI_U.h -> World_U.h -> Entity_U.h 순환 방지

class EnemyAI
{
public:
    void Update(World& tWorld, Handle hPlayer, float fDt) const;
};
