#pragma once

#include "core/Body_C.h"
#include "core/MoveIntent_C.h"

// 점프 보조 상태
struct JumpState
{
    float fCoyoteTimer = 0.0f;  // > 0 이면 땅에서 막 벗어났어도 점프 가능
    float fBufferTimer = 0.0f;  // > 0 이면 최근에 점프를 눌렀음
    bool bIsRising = false;     // 점프로 상승 중 (가변 점프 컷 대상)
};

// 대시(빨리 달리기) 상태
struct DashState
{
    float fGauge = 1.0f;      // 0 ~ 1
    bool bExhausted = false;  // 다 써서 탈진. fGauge가 fDashRecoverRatio 이상 될 때까지 대시 금지
    bool bActive = false;     // 이번 틱에 대시 중인지
};

// 플레이어 이동 관련 상태 묶음. PlayerMotor만 수정함.
struct PlayerMotorState
{
    JumpState tJump;
    DashState tDash;
    int iFacing = 1;  // 바라보는 방향 -1 / +1. 멈춰도 마지막 방향 유지
};

// 플레이어 이동 의도 -> 속도. 위치는 건드리지 않음 (이동·충돌은 Physics).
class PlayerMotor
{
public:
    // fMoveSpeed: Stats 최종값 (지금은 PlayerDef 기본값)
    void Update(const MoveIntent& tIntent, Body& tBody, PlayerMotorState& tState, float fMoveSpeed,
                float fDt) const;
};
