#pragma once

#include "core/Vec2_C.h"

// 적 튜닝 값. 지금은 Chaser 하나 (오늘은 AI 없는 허수아비로 사용).
struct EnemyDef
{
    Vec2 vSize = {28.0f, 36.0f};
    float fMaxHp = 30.0f;        // 플레이어 공격 3번
    float fMoveSpeed = 140.0f;   // Chaser AI 때 사용. 지금은 넉백 후 감속에만 쓰임
    float fAttackDamage = 10.0f; // Chaser 공격 때 사용
};

inline constexpr EnemyDef kChaserDef{};
