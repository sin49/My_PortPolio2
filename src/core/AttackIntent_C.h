#pragma once

// 공격 의도. Input / EnemyAI가 기록하고, Combat만 읽음.
struct AttackIntent
{
    bool bAttackPressed = false;  // 눌린 순간. 래치 -> 고정 틱이 소비 후 false
};
