#pragma once

#include "core/Vec2_C.h"
#include "data/AttackDef_C.h"

// 플레이어 튜닝 값 (코드에 매직넘버 X). 숫자는 플레이해보며 조정.
struct PlayerDef
{
    Vec2 vSize = {24.0f, 40.0f};
    float fMoveSpeed = 340.0f;          // 기본값 (11/17 사용자 요청으로 260 -> 340). TODO(Stats): 최종값은 Stats에서

    // 수평 관성: "0 -> 최고 속도"(또는 걷기 속도 -> 0)까지 걸리는 시간(초)으로 조정.
    float fGroundAccelTime = 0.15f;  // 걷기 출발: 살짝 느리게 시작해서 0.15초 뒤 최고 속도
    float fDashAccelTime = 0.08f;    // 대시 누른 채 출발: 대시 속도까지 더 빨리 붙음
    float fGroundStopTime = 0.17f;   // 키 떼면 미끄러지며 멈춤 (걷기 기준 약 29px < 1칸, 달리던 중이면 더 김)
    float fAirAccelTime = 0.15f;
    float fAirStopTime = 0.50f;      // 공중에선 더 오래 미끄러짐

    float fJumpSpeed = 880.0f;          // 최고 높이 ≈ v² / 2g = 161px (약 5칸). 11/17 사용자 요청으로 760 -> 880
    float fJumpCutMultiplier = 0.5f;    // 상승 중 키를 떼면 vy에 한 번 곱함
    float fCoyoteTime = 0.10f;          // 발판을 벗어난 뒤에도 점프 가능한 시간
    float fJumpBufferTime = 0.12f;      // 착지 전에 미리 누른 점프를 기억하는 시간

    // Shift: 짧게 눌렀다 떼면 = 대시(짧고 빠른 전진 + 짧은 무적), 길게 누르고 있으면 = 달리기. 둘 다 게이지(0~1) 사용.
    float fDashTapTime = 0.18f;         // 이 시간 안에 떼면 대시, 넘게 누르고 있으면 달리기 시작
    // 장거리는 달리기가 유리: 게이지 0.5로 대시 ≈ 126px / 달리기 ≈ 1530px
    float fRunSpeedMultiplier = 1.8f;   // 340 -> 612 px/s (틱당 10px)
    float fRunDrainPerSec = 0.2f;       // 꽉 찬 게이지로 5초 (기동성 우선, 사용자 요청)
    float fRunJumpMultiplier = 1.15f;   // 달리는 중 점프 속도 배율 -> 높이 약 1.32배 (약 213px, 6.7칸)
    float fDashSpeed = 900.0f;          // 수평만 (위아래 X, 대시 중 중력 없음). 틱당 15px < 타일 32px
    float fDashTime = 0.14f;            // 거리 = 900 * 0.14 ≈ 126px (약 4칸)
    float fDashCost = 0.5f;             // 게이지 절반
    float fDashInvulnTime = 0.10f;      // 대시 시작부터 무적
    float fDashRegenPerSec = 0.8f;      // 대시·달리기 안 할 때 회복. 0 -> 0.5 회복 0.625초 (대시 약 0.6초마다 가능)
    float fDashRecoverRatio = 0.5f;     // 탈진 해제 기준

    // 근접 공격: 크기 40x32, 유지 0.15초, 대기 0.2초, 버퍼 0.12초. TODO(Stats): 범위 modifier
    AttackDef tAttack = {.vSize = {40.0f, 32.0f}, .fActiveTime = 0.15f, .fRecoverTime = 0.20f, .fBufferTime = 0.12f};

    // HP·피격
    float fMaxHp = 100.0f;              // TODO(Stats): 최대 HP modifier
    float fAttackDamage = 10.0f;        // TODO(Stats): 공격력 modifier
    float fKnockbackSpeed = 320.0f;     // 맞은 쪽 수평 속도 (Motor 감속으로 미끄러지며 멈춤)
    float fKnockbackUp = 220.0f;        // 살짝 뜸
    float fHitFlashTime = 0.10f;        // 맞은 쪽 흰색 깜빡임
    float fHurtInvulnTime = 0.6f;       // 플레이어 피격 뒤 무적 (겹친 적 여러 마리에게 한꺼번에 맞지 않게, 11/19)
    int iHitStopTicks = 4;              // 플레이어가 맞으면 게임을 이만큼 고정 틱 멈춤 (히트스톱, 4틱 ≈ 0.067초)
};

inline constexpr PlayerDef kPlayerDef{};
