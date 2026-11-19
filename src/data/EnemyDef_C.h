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

// Shooter: 거리를 유지하며 플레이어 쪽으로 1발 (shooter.md). 크기·HP 고정 (중간, 2대)
struct ShooterDef
{
    Vec2 vSize = {28.0f, 38.0f};
    float fMaxHp = 20.0f;
    float fMoveSpeed = 110.0f;

    float fDetectRange = 560.0f;  // Idle -> Chase (가로·세로 둘 다)
    float fLoseRange = 760.0f;    // 가로가 이보다 멀면 Chase -> Idle
    float fPreferMin = 220.0f;    // 가로 거리가 이보다 가까우면 물러남
    float fPreferMax = 380.0f;    // 이보다 멀면 다가감. 이 안이면 Windup (쏠 준비)
    float fFireRangeY = 260.0f;   // 세로 차이가 이 안일 때만 쏨 (너무 위·아래 층이면 안 쏨)
    float fWindupTime = 0.70f;    // 발사 예고 (깜빡임). 11/19 사용자: 덜 자주 쏘게 0.5 -> 0.7
    float fRecoverTime = 2.40f;   // 쏜 뒤 쉬는 시간 (이동은 함). 11/19 사용자: 1.4 -> 2.4
};

inline constexpr ShooterDef kShooterDef{};

// 보스 (boss-projectile.md): 조준 3연사 <-> 8방향 원형탄 번갈아, HP 절반 아래면 쉬는 시간이 짧아짐 (2페이즈)
struct BossDef
{
    Vec2 vSize = {64.0f, 72.0f};
    float fMaxHp = 100.0f;      // 플레이어 기본 공격 10번 (11/19 사용자: 포폴용이라 300 -> 100)
    float fMoveSpeed = 60.0f;   // Recover 때만 천천히 다가옴
    float fDetectRange = 900.0f;  // 가로 거리가 이 안이면 Idle -> 첫 패턴 (방 입장 직후 숨 돌릴 틈)

    // 11/19 사용자: "충분히 어렵다" -> 공격 빈도 낮춤 (예고 길게, 쉼 길게)
    float fAimedWindupTime = 0.7f;
    int iAimedShots = 3;
    float fAimedSpreadDeg = 24.0f;  // 가운데 기준 -12°, 0°, +12°
    float fRingWindupTime = 0.9f;
    int iRingShots = 8;             // 45°마다
    float fRecoverTime = 2.0f;
    float fRecoverTimePhase2 = 1.3f;
    float fPhase2HpRatio = 0.5f;    // HP가 이 비율 이하면 2페이즈
};

inline constexpr BossDef kBossDef{};
