#include "systems/PlayerMotor_C.h"

#include "data/PlayerDef_C.h"

#include <algorithm>
#include <cmath>

namespace
{
// fCurrent를 fTarget 쪽으로 최대 fMaxDelta만큼 이동 (넘어가지 않음). Unity의 Mathf.MoveTowards.
float MoveToward(float fCurrent, float fTarget, float fMaxDelta)
{
    if (fCurrent < fTarget) return std::min(fCurrent + fMaxDelta, fTarget);
    if (fCurrent > fTarget) return std::max(fCurrent - fMaxDelta, fTarget);
    return fTarget;
}

// Shift: 짧게(fDashTapTime 안에) 눌렀다 떼면 뗀 순간 대시 / 그보다 오래 누르고 있고 방향키가 있으면 달리기.
// 방향키가 없으면 달리기 아님 = 게이지도 안 씀.
void UpdateDash(const MoveIntent& tIntent, DashState& tDash, int iFacing, float fDt)
{
    tDash.fInvulnTimer = std::max(0.0f, tDash.fInvulnTimer - fDt);
    tDash.fDashTimer = std::max(0.0f, tDash.fDashTimer - fDt);

    // 누름 시작: 눌린 순간 래치(bDashPressed) 또는 지난 틱엔 안 눌렸는데 지금 눌림.
    // 래치를 같이 보는 이유: 두 고정 틱 사이에 눌렀다 뗀 아주 짧은 탭도 놓치지 않으려고
    const bool bPressStart = !tDash.bWasHeld && (tIntent.bDashPressed || tIntent.bDashHeld);
    if (bPressStart)
        tDash.fHoldTimer = 0.0f;
    if (tIntent.bDashHeld)
        tDash.fHoldTimer += fDt;
    const bool bReleased = (tDash.bWasHeld || bPressStart) && !tIntent.bDashHeld;
    const bool bTapped = bReleased && tDash.fHoldTimer <= kPlayerDef.fDashTapTime;
    tDash.bWasHeld = tIntent.bDashHeld;

    const bool bCanStartDash = !tDash.IsDashing() && !tDash.bExhausted && tDash.fGauge >= kPlayerDef.fDashCost;
    if (bTapped && bCanStartDash)
    {
        tDash.fGauge -= kPlayerDef.fDashCost;
        tDash.fDashTimer = kPlayerDef.fDashTime;
        tDash.fInvulnTimer = kPlayerDef.fDashInvulnTime;
        tDash.iDashDir = tIntent.iMoveDir != 0 ? tIntent.iMoveDir : iFacing;  // 누르는 방향, 없으면 마지막 방향
        if (tDash.fGauge <= 0.0f)
        {
            tDash.fGauge = 0.0f;
            tDash.bExhausted = true;
        }
    }

    const bool bLongHold = tIntent.bDashHeld && tDash.fHoldTimer > kPlayerDef.fDashTapTime;
    tDash.bRunning = !tDash.IsDashing() && bLongHold && tIntent.iMoveDir != 0 && !tDash.bExhausted;
    if (tDash.bRunning)
    {
        tDash.fGauge -= kPlayerDef.fRunDrainPerSec * fDt;
        if (tDash.fGauge <= 0.0f)
        {
            tDash.fGauge = 0.0f;
            tDash.bExhausted = true;
            tDash.bRunning = false;
        }
    }
    else if (!tDash.IsDashing())
    {
        tDash.fGauge = std::min(1.0f, tDash.fGauge + kPlayerDef.fDashRegenPerSec * fDt);
        if (tDash.bExhausted && tDash.fGauge >= kPlayerDef.fDashRecoverRatio)
            tDash.bExhausted = false;
    }
}

// 수평 관성: 입력이 있으면 가속, 없으면 감속 (지상/공중 값 따로). 대시 중엔 고정 속도.
void UpdateHorizontal(const MoveIntent& tIntent, Body& tBody, const DashState& tDash, float fMoveSpeed, float fDt)
{
    if (tDash.IsDashing())
    {
        tBody.vVel.fX = static_cast<float>(tDash.iDashDir) * kPlayerDef.fDashSpeed;
        return;
    }

    const float fMaxSpeed = tDash.bRunning ? fMoveSpeed * kPlayerDef.fRunSpeedMultiplier : fMoveSpeed;
    const float fTargetSpeed = static_cast<float>(tIntent.iMoveDir) * fMaxSpeed;

    // fRate(px/s²) = 속도 / 걸리는 시간
    float fRate = 0.0f;
    if (tIntent.iMoveDir == 0)
        fRate = fMoveSpeed / (tBody.bGrounded ? kPlayerDef.fGroundStopTime : kPlayerDef.fAirStopTime);
    else if (tDash.bRunning)
        fRate = fMaxSpeed / kPlayerDef.fDashAccelTime;
    else
        fRate = fMoveSpeed / (tBody.bGrounded ? kPlayerDef.fGroundAccelTime : kPlayerDef.fAirAccelTime);

    // 대시 직후처럼 최고 속도보다 빠르면 감속 쪽 비율로 내려옴 (뚝 끊기지 않게)
    if (std::abs(tBody.vVel.fX) > fMaxSpeed)
        fRate = std::max(fRate, fMoveSpeed / kPlayerDef.fGroundStopTime);

    tBody.vVel.fX = MoveToward(tBody.vVel.fX, fTargetSpeed, fRate * fDt);
}

// 반환: 이번 틱에 점프했는지
bool UpdateJump(const MoveIntent& tIntent, Body& tBody, JumpState& tJump, bool bRunning, float fDt)
{
    // 타이머 (고정 dt에서만 감소 -> 프레임레이트와 무관)
    tJump.fCoyoteTimer = tBody.bGrounded ? kPlayerDef.fCoyoteTime : std::max(0.0f, tJump.fCoyoteTimer - fDt);
    tJump.fBufferTimer =
        tIntent.bJumpPressed ? kPlayerDef.fJumpBufferTime : std::max(0.0f, tJump.fBufferTimer - fDt);

    bool bJumped = false;
    // 점프: 최근에 눌렀고(버퍼) + 땅이거나 막 벗어났으면(코요테)
    if (tJump.fBufferTimer > 0.0f && tJump.fCoyoteTimer > 0.0f)
    {
        const float fMultiplier = bRunning ? kPlayerDef.fRunJumpMultiplier : 1.0f;  // 달리면서 점프 = 더 높이
        tBody.vVel.fY = -kPlayerDef.fJumpSpeed * fMultiplier;
        tJump.fBufferTimer = 0.0f;
        tJump.fCoyoteTimer = 0.0f;  // 공중에서 코요테 점프를 다시 쓰지 못하게
        tJump.bIsRising = true;
        bJumped = true;
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
    return bJumped;
}
} // namespace

void PlayerMotor::Update(const MoveIntent& tIntent, Body& tBody, PlayerMotorState& tState, float fMoveSpeed,
                         float fDt) const
{
    if (tIntent.iMoveDir != 0)
        tState.iFacing = tIntent.iMoveDir;

    UpdateDash(tIntent, tState.tDash, tState.iFacing, fDt);
    UpdateHorizontal(tIntent, tBody, tState.tDash, fMoveSpeed, fDt);

    if (tState.tDash.IsDashing())
    {
        tState.iFacing = tState.tDash.iDashDir;
        tBody.vVel.fY = 0.0f;  // 대시는 수평만: 떨어지지도 않음
    }

    const bool bJumped = UpdateJump(tIntent, tBody, tState.tJump, tState.tDash.bRunning, fDt);
    if (bJumped)
        tState.tDash.fDashTimer = 0.0f;  // 대시 중 점프 = 대시 끊고 점프 (무적은 남은 만큼 유지)

    tBody.bNoGravity = tState.tDash.IsDashing();
}
