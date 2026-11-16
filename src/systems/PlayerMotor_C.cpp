#include "systems/PlayerMotor_C.h"

#include "data/PlayerDef_C.h"

#include <algorithm>

namespace
{
// fCurrent를 fTarget 쪽으로 최대 fMaxDelta만큼 이동 (넘어가지 않음). Unity의 Mathf.MoveTowards.
float MoveToward(float fCurrent, float fTarget, float fMaxDelta)
{
    if (fCurrent < fTarget) return std::min(fCurrent + fMaxDelta, fTarget);
    if (fCurrent > fTarget) return std::max(fCurrent - fMaxDelta, fTarget);
    return fTarget;
}

void UpdateDash(const MoveIntent& tIntent, DashState& tDash, float fDt)
{
    const bool bWantsDash = tIntent.bDashHeld && tIntent.iMoveDir != 0 && !tDash.bExhausted;
    if (bWantsDash)
    {
        tDash.bActive = true;
        tDash.fGauge -= kPlayerDef.fDashDrainPerSec * fDt;
        if (tDash.fGauge <= 0.0f)
        {
            tDash.fGauge = 0.0f;
            tDash.bExhausted = true;
            tDash.bActive = false;
        }
    }
    else
    {
        tDash.bActive = false;
        tDash.fGauge = std::min(1.0f, tDash.fGauge + kPlayerDef.fDashRegenPerSec * fDt);
        if (tDash.bExhausted && tDash.fGauge >= kPlayerDef.fDashRecoverRatio)
            tDash.bExhausted = false;
    }
}

// 수평 관성: 입력이 있으면 가속, 없으면 감속 (지상/공중 값 따로)
void UpdateHorizontal(const MoveIntent& tIntent, Body& tBody, const DashState& tDash, float fMoveSpeed, float fDt)
{
    const float fMaxSpeed = tDash.bActive ? fMoveSpeed * kPlayerDef.fDashSpeedMultiplier : fMoveSpeed;
    const float fTargetSpeed = static_cast<float>(tIntent.iMoveDir) * fMaxSpeed;

    // fRate(px/s²) = 속도 / 걸리는 시간
    float fRate = 0.0f;
    if (tIntent.iMoveDir == 0)
        fRate = fMoveSpeed / (tBody.bGrounded ? kPlayerDef.fGroundStopTime : kPlayerDef.fAirStopTime);
    else if (tDash.bActive)
        fRate = fMaxSpeed / kPlayerDef.fDashAccelTime;
    else
        fRate = fMoveSpeed / (tBody.bGrounded ? kPlayerDef.fGroundAccelTime : kPlayerDef.fAirAccelTime);

    tBody.vVel.fX = MoveToward(tBody.vVel.fX, fTargetSpeed, fRate * fDt);
}

void UpdateJump(const MoveIntent& tIntent, Body& tBody, JumpState& tJump, float fDt)
{
    // 타이머 (고정 dt에서만 감소 -> 프레임레이트와 무관)
    tJump.fCoyoteTimer = tBody.bGrounded ? kPlayerDef.fCoyoteTime : std::max(0.0f, tJump.fCoyoteTimer - fDt);
    tJump.fBufferTimer =
        tIntent.bJumpPressed ? kPlayerDef.fJumpBufferTime : std::max(0.0f, tJump.fBufferTimer - fDt);

    // 점프: 최근에 눌렀고(버퍼) + 땅이거나 막 벗어났으면(코요테)
    if (tJump.fBufferTimer > 0.0f && tJump.fCoyoteTimer > 0.0f)
    {
        tBody.vVel.fY = -kPlayerDef.fJumpSpeed;
        tJump.fBufferTimer = 0.0f;
        tJump.fCoyoteTimer = 0.0f;  // 공중에서 코요테 점프를 다시 쓰지 못하게
        tJump.bIsRising = true;
    }

    // 가변 점프: 상승 중에 키를 떼면 한 번만 감속
    if (tJump.bIsRising)
    {
        if (tBody.vVel.fY >= 0.0f)
        {
            tJump.bIsRising = false;  // 정점을 지났거나 천장에 박음
        }
        else if (!tIntent.bJumpHeld)
        {
            tBody.vVel.fY *= kPlayerDef.fJumpCutMultiplier;
            tJump.bIsRising = false;
        }
    }
}
} // namespace

void PlayerMotor::Update(const MoveIntent& tIntent, Body& tBody, PlayerMotorState& tState, float fMoveSpeed,
                         float fDt) const
{
    if (tIntent.iMoveDir != 0)
        tState.iFacing = tIntent.iMoveDir;

    UpdateDash(tIntent, tState.tDash, fDt);
    UpdateHorizontal(tIntent, tBody, tState.tDash, fMoveSpeed, fDt);
    UpdateJump(tIntent, tBody, tState.tJump, fDt);
}
