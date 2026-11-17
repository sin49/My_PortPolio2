#pragma once

#include "core/Vec2_C.h"

// 플레이어 튜닝 값 (코드에 매직넘버 X). 숫자는 플레이해보며 조정.
struct PlayerDef
{
    Vec2 vSize = {24.0f, 40.0f};
    float fMoveSpeed = 260.0f;          // 기본값. TODO(Stats): 최종값은 Stats(기본값 + modifier)에서

    // 수평 관성: "0 -> 최고 속도"(또는 걷기 속도 -> 0)까지 걸리는 시간(초)으로 조정.
    float fGroundAccelTime = 0.15f;  // 걷기 출발: 살짝 느리게 시작해서 0.15초 뒤 최고 속도
    float fDashAccelTime = 0.08f;    // 대시 누른 채 출발: 대시 속도까지 더 빨리 붙음
    float fGroundStopTime = 0.19f;   // 키 떼면 미끄러지며 멈춤 (걷기 기준 약 24px, 대시 중이면 더 김)
    float fAirAccelTime = 0.15f;
    float fAirStopTime = 0.50f;      // 공중에선 더 오래 미끄러짐

    float fJumpSpeed = 760.0f;          // 최고 높이 ≈ v² / 2g = 120px (약 3.75칸)
    float fJumpCutMultiplier = 0.5f;    // 상승 중 키를 떼면 vy에 한 번 곱함
    float fCoyoteTime = 0.10f;          // 발판을 벗어난 뒤에도 점프 가능한 시간
    float fJumpBufferTime = 0.12f;      // 착지 전에 미리 누른 점프를 기억하는 시간

    // 대시 = 누르는 동안 빨리 달리기. 게이지는 0~1 비율.
    float fDashSpeedMultiplier = 1.8f;  // 260 -> 468 px/s
    float fDashDrainPerSec = 0.5f;      // 꽉 찬 게이지로 2초
    float fDashRegenPerSec = 0.4f;      // 0 -> 0.5 회복 1.25초
    float fDashRecoverRatio = 0.5f;     // 탈진 해제 기준

    // 근접 공격
    Vec2 vAttackSize = {40.0f, 32.0f};  // TODO(Stats): 범위 modifier
    float fAttackActiveTime = 0.15f;    // 히트박스 유지
    float fAttackRecoverTime = 0.20f;   // 다음 공격까지 대기
    float fAttackBufferTime = 0.12f;    // 대기 중 미리 누른 공격 기억

    // HP·피격
    float fMaxHp = 100.0f;              // TODO(Stats): 최대 HP modifier
    float fAttackDamage = 10.0f;        // TODO(Stats): 공격력 modifier
    float fKnockbackSpeed = 320.0f;     // 맞은 쪽 수평 속도 (Motor 감속으로 미끄러지며 멈춤)
    float fKnockbackUp = 220.0f;        // 살짝 뜸
    float fHitFlashTime = 0.10f;        // 맞은 쪽 흰색 깜빡임
};

inline constexpr PlayerDef kPlayerDef{};
