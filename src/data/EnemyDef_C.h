#pragma once

#include "core/Vec2_C.h"
#include "data/AttackDef_C.h"

// 적 튜닝 값. 지금은 Chaser 하나. 거리 기준은 docs/design/chaser.md 3장
struct EnemyDef
{
    Vec2 vSize = {28.0f, 36.0f};
    float fMaxHp = 30.0f;        // 플레이어 공격 3번
    float fMoveSpeed = 140.0f;
    float fAttackDamage = 10.0f;

    // AI (EnemyAI가 읽음)
    float fDetectRange = 420.0f;  // 중심 사이 가로·세로 거리가 둘 다 이 안이면 Idle -> Chase
    float fLoseRange = 640.0f;    // 가로 거리가 이보다 멀어지면 Chase -> Idle
    float fAttackRangeX = 36.0f;  // 몸 끝 사이 가로 간격이 이하이고
    float fAttackRangeY = 40.0f;  // 중심 세로 차이가 이하면 (같은 층) Chase -> Windup
    float fWindupTime = 0.40f;    // 공격 예고
    float fRecoverTime = 0.60f;   // 공격 뒤 멈춤 (플레이어 반격 타이밍)

    AttackDef tAttack = {.vSize = {36.0f, 28.0f}, .fActiveTime = 0.12f, .fRecoverTime = 0.10f, .fBufferTime = 0.0f};
};

inline constexpr EnemyDef kChaserDef{};

// 크기 = 맞아야 죽는 횟수 (플레이어 공격력 10 기준 1·2·3대). 작은 놈이 제일 많게 배치 (Rooms_C.h)
enum class EnemySize
{
    Small,   // 1대
    Medium,  // 2대
    Large,   // 3대
};

struct EnemySizeDef
{
    Vec2 vSize = {};
    float fMaxHp = 0.0f;
};

inline constexpr EnemySizeDef kEnemySizes[] = {
    {.vSize = {22.0f, 28.0f}, .fMaxHp = 10.0f},  // Small
    {.vSize = {28.0f, 38.0f}, .fMaxHp = 20.0f},  // Medium
    {.vSize = {38.0f, 50.0f}, .fMaxHp = 30.0f},  // Large
};

inline constexpr const EnemySizeDef& GetEnemySizeDef(EnemySize eSize)
{
    return kEnemySizes[static_cast<int>(eSize)];
}
