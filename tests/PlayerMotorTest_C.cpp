#include "core/Body_C.h"
#include "core/FixedTimestep_C.h"
#include "core/MoveIntent_C.h"
#include "core/Tilemap_C.h"
#include "data/PlayerDef_C.h"
#include "systems/Physics_C.h"
#include "systems/PlayerMotor_C.h"

#include <algorithm>
#include <array>
#include <span>
#include <string_view>

#include <doctest/doctest.h>

namespace
{
constexpr float kDt = FixedTimestep::kStep;

// 20 x 10: 행 3에 왼쪽 절반 발판(윗면 y=96, 오른쪽 끝 x=320), 행 9 바닥(윗면 y=288)
constexpr std::array<std::string_view, 10> kLedgeMap = {
    ".........." "..........",
    ".........." "..........",
    ".........." "..........",
    "##########" "..........",
    ".........." "..........",
    ".........." "..........",
    ".........." "..........",
    ".........." "..........",
    ".........." "..........",
    "##########" "##########",
};

// 20 x 12: 바닥만 (윗면 y=352). 점프 높이가 천장에 안 막히게 넉넉히.
constexpr std::array<std::string_view, 12> kFloorMap = {
    ".........." "..........",
    ".........." "..........",
    ".........." "..........",
    ".........." "..........",
    ".........." "..........",
    ".........." "..........",
    ".........." "..........",
    ".........." "..........",
    ".........." "..........",
    ".........." "..........",
    ".........." "..........",
    "##########" "##########",
};

constexpr float kFloorTop = 352.0f;

// 고정 틱 한 번 = Game::FixedUpdate와 같은 순서
struct Sim
{
    Tilemap tMap;
    Body tBody;
    PlayerMotorState tState;
    PlayerMotor tMotor;
    Physics tPhysics;

    void Tick(const MoveIntent& tIntent)
    {
        tMotor.Update(tIntent, tBody, tState, kPlayerDef.fMoveSpeed, kDt);
        tPhysics.Update(tBody, tMap, kDt);
    }
};

constexpr MoveIntent kIdle = {};
constexpr MoveIntent kHold = {.bJumpHeld = true};
constexpr MoveIntent kPress = {.bJumpPressed = true, .bJumpHeld = true};
constexpr MoveIntent kTap = {.bJumpPressed = true, .bJumpHeld = false};
constexpr MoveIntent kWalkRight = {.iMoveDir = 1};
constexpr MoveIntent kWalkRightPress = {.iMoveDir = 1, .bJumpPressed = true, .bJumpHeld = true};
constexpr MoveIntent kWalkLeft = {.iMoveDir = -1};
constexpr MoveIntent kWalkRightHold = {.iMoveDir = 1, .bJumpHeld = true};
constexpr MoveIntent kRunRight = {.iMoveDir = 1, .bDashHeld = true};
constexpr MoveIntent kRunOnly = {.bDashHeld = true};
constexpr MoveIntent kRunRightJumpPress = {.iMoveDir = 1, .bJumpPressed = true, .bJumpHeld = true, .bDashHeld = true};
constexpr MoveIntent kRunRightJumpHold = {.iMoveDir = 1, .bJumpHeld = true, .bDashHeld = true};

Sim MakeSim(std::span<const std::string_view> spRows, Vec2 vPos)
{
    return Sim{.tMap = *Tilemap::FromRows(spRows), .tBody = {.vPos = vPos, .vSize = kPlayerDef.vSize}};
}

// 바닥 위에 서 있는 상태
Sim MakeStanding()
{
    return MakeSim(kFloorMap, {100.0f, kFloorTop - kPlayerDef.vSize.fY});
}

// 발판 오른쪽 끝에 세운 뒤, 오른쪽으로 걸어서 떨어지기 시작한 틱까지 진행
Sim WalkOffLedge()
{
    Sim tSim = MakeSim(kLedgeMap, {296.0f, 96.0f - kPlayerDef.vSize.fY});
    tSim.Tick(kIdle);
    REQUIRE(tSim.tBody.bGrounded);

    for (int i = 0; i < 60 && tSim.tBody.bGrounded; ++i)
        tSim.Tick(kWalkRight);
    REQUIRE_FALSE(tSim.tBody.bGrounded);
    return tSim;
}

// 바닥 위 150px에서 떨어뜨림
Sim MakeFalling()
{
    return MakeSim(kFloorMap, {100.0f, kFloorTop - kPlayerDef.vSize.fY - 150.0f});
}

int CountTicksUntilLanding()
{
    Sim tSim = MakeFalling();
    for (int iTick = 1; iTick <= 120; ++iTick)
    {
        tSim.Tick(kIdle);
        if (tSim.tBody.bGrounded) return iTick;
    }
    return -1;
}

// 바닥에 선 상태에서 첫 틱 입력 후 계속 다음 입력. 최고 높이(px) 반환.
float MeasureJumpHeight(const MoveIntent& tFirst, const MoveIntent& tAfter)
{
    Sim tSim = MakeStanding();
    tSim.Tick(kIdle);
    REQUIRE(tSim.tBody.bGrounded);

    const float fStartY = tSim.tBody.vPos.fY;
    float fMinY = fStartY;
    tSim.Tick(tFirst);
    for (int i = 0; i < 90; ++i)
    {
        fMinY = std::min(fMinY, tSim.tBody.vPos.fY);
        tSim.Tick(tAfter);
    }
    return fStartY - fMinY;
}
} // namespace

TEST_CASE("PlayerMotor: 코요테 - 발판을 벗어난 직후 점프는 성공")
{
    Sim tSim = WalkOffLedge();
    tSim.Tick(kWalkRight);
    tSim.Tick(kWalkRight);
    tSim.Tick(kWalkRightPress);
    CHECK(tSim.tBody.vVel.fY < 0.0f);
}

TEST_CASE("PlayerMotor: 코요테 - 시간이 지나면 점프 실패")
{
    Sim tSim = WalkOffLedge();
    for (int i = 0; i < 10; ++i)  // 약 0.17초 > fCoyoteTime 0.10초
        tSim.Tick(kWalkRight);
    tSim.Tick(kWalkRightPress);
    CHECK(tSim.tBody.vVel.fY > 0.0f);
}

TEST_CASE("PlayerMotor: 코요테 점프 후 공중에서 다시 점프 못 함")
{
    Sim tSim = WalkOffLedge();
    tSim.Tick(kWalkRightPress);
    REQUIRE(tSim.tBody.vVel.fY < 0.0f);

    for (int i = 0; i < 120 && tSim.tBody.vVel.fY <= 0.0f; ++i)
        tSim.Tick(kHold);
    REQUIRE(tSim.tBody.vVel.fY > 0.0f);
    REQUIRE_FALSE(tSim.tBody.bGrounded);

    tSim.Tick(kPress);
    CHECK(tSim.tBody.vVel.fY > 0.0f);
}

TEST_CASE("PlayerMotor: 점프 버퍼 - 착지 직전 입력은 착지 후 점프로 실행")
{
    const int iLanding = CountTicksUntilLanding();
    REQUIRE(iLanding > 15);

    Sim tSim = MakeFalling();
    bool bJumped = false;
    for (int iTick = 1; iTick <= iLanding + 5; ++iTick)
    {
        tSim.Tick(iTick == iLanding - 3 ? kPress : kHold);
        if (iTick > iLanding - 3 && tSim.tBody.vVel.fY < 0.0f) bJumped = true;
    }
    CHECK(bJumped);
}

TEST_CASE("PlayerMotor: 점프 버퍼 - 너무 일찍 누른 입력은 버려짐")
{
    const int iLanding = CountTicksUntilLanding();
    REQUIRE(iLanding > 15);

    Sim tSim = MakeFalling();
    bool bJumped = false;
    for (int iTick = 1; iTick <= iLanding + 5; ++iTick)
    {
        tSim.Tick(iTick == iLanding - 15 ? kPress : kHold);  // 0.25초 전 > fJumpBufferTime 0.12초
        if (tSim.tBody.vVel.fY < 0.0f) bJumped = true;
    }
    CHECK_FALSE(bJumped);
    CHECK(tSim.tBody.bGrounded);
}

TEST_CASE("PlayerMotor: 관성 - 가속해서 최고 속도에 도달")
{
    Sim tSim = MakeStanding();
    tSim.Tick(kIdle);
    REQUIRE(tSim.tBody.bGrounded);

    tSim.Tick(kWalkRight);
    CHECK(tSim.tBody.vVel.fX > 0.0f);
    CHECK(tSim.tBody.vVel.fX < kPlayerDef.fMoveSpeed);  // 첫 틱에 바로 최고 속도 아님

    for (int i = 0; i < 30; ++i)
        tSim.Tick(kWalkRight);
    CHECK(tSim.tBody.vVel.fX == doctest::Approx(kPlayerDef.fMoveSpeed));
}

TEST_CASE("PlayerMotor: 관성 - 키를 떼면 살짝 미끄러지다 멈춤")
{
    Sim tSim = MakeStanding();
    for (int i = 0; i < 30; ++i)
        tSim.Tick(kWalkRight);
    REQUIRE(tSim.tBody.vVel.fX == doctest::Approx(kPlayerDef.fMoveSpeed));

    const float fReleaseX = tSim.tBody.vPos.fX;
    tSim.Tick(kIdle);
    CHECK(tSim.tBody.vVel.fX > 0.0f);  // 바로 0이 아님

    for (int i = 0; i < 60; ++i)
        tSim.Tick(kIdle);
    CHECK(tSim.tBody.vVel.fX == 0.0f);

    const float fSlide = tSim.tBody.vPos.fX - fReleaseX;
    CHECK(fSlide > 5.0f);
    CHECK(fSlide < kTileSize);  // 한 칸 넘게 미끄러지진 않음
}

// ---------- Shift: 짧게 눌렀다 떼면 대시 / 길게 누르면 달리기 ----------
namespace
{
// fDashTapTime(0.18초)을 넘기는 틱 수
const int kLongHoldTicks = static_cast<int>(kPlayerDef.fDashTapTime / kDt) + 2;

// 짧게 탭: 2틱 누르고 떼기. 대시는 뗀 틱에 나감
void TapDash(Sim& tSim, int iDir)
{
    tSim.Tick({.iMoveDir = iDir, .bDashPressed = true, .bDashHeld = true});
    tSim.Tick({.iMoveDir = iDir, .bDashHeld = true});
    tSim.Tick({.iMoveDir = iDir});
}

void HoldRun(Sim& tSim, int iTicks)
{
    for (int i = 0; i < iTicks; ++i)
        tSim.Tick(kRunRight);
}
} // namespace

TEST_CASE("PlayerMotor: 달리기 - 길게 누르면 최고 속도가 배율만큼 빨라짐")
{
    Sim tSim = MakeStanding();
    HoldRun(tSim, 40);
    CHECK(tSim.tState.tDash.bRunning);
    CHECK(tSim.tState.tDash.fGauge < 1.0f);
    CHECK(tSim.tBody.vVel.fX == doctest::Approx(kPlayerDef.fMoveSpeed * kPlayerDef.fRunSpeedMultiplier));
}

TEST_CASE("PlayerMotor: 달리기 - 탭 판정 시간 안에는 아직 걷기 (게이지 안 씀)")
{
    Sim tSim = MakeStanding();
    HoldRun(tSim, kLongHoldTicks - 4);
    CHECK_FALSE(tSim.tState.tDash.bRunning);
    CHECK(tSim.tState.tDash.fGauge == 1.0f);
    HoldRun(tSim, 4);
    CHECK(tSim.tState.tDash.bRunning);
}

TEST_CASE("PlayerMotor: 달리기 - 길게 누르다 떼면 대시 안 나감")
{
    Sim tSim = MakeStanding();
    HoldRun(tSim, kLongHoldTicks + 10);
    tSim.Tick(kWalkRight);  // 뗌
    CHECK_FALSE(tSim.tState.tDash.IsDashing());
}

TEST_CASE("PlayerMotor: 달리기 - 다 쓰면 탈진, 절반 회복 후 다시 가능")
{
    Sim tSim = MakeStanding();
    int iTicks = 0;
    while (!tSim.tState.tDash.bExhausted && iTicks < 500)  // 탭 판정 ~11틱 + 게이지 / 0.2 = 5초 ≈ 300틱
    {
        tSim.Tick(kRunRight);
        ++iTicks;
    }
    REQUIRE(tSim.tState.tDash.bExhausted);
    CHECK(iTicks == doctest::Approx(300 + kLongHoldTicks).epsilon(0.03));
    CHECK(tSim.tState.tDash.fGauge == 0.0f);

    tSim.Tick(kRunRight);  // 탈진 중에는 눌러도 달리기 안 됨 (회복은 됨)
    CHECK_FALSE(tSim.tState.tDash.bRunning);

    for (int i = 0; i < 20; ++i)  // 누적 약 0.35초 회복 -> 0.28 < 0.5
        tSim.Tick(kRunRight);
    CHECK(tSim.tState.tDash.bExhausted);

    for (int i = 0; i < 25; ++i)  // 누적 약 0.77초 -> 0.61 >= 0.5
        tSim.Tick(kIdle);
    CHECK_FALSE(tSim.tState.tDash.bExhausted);

    HoldRun(tSim, kLongHoldTicks + 1);
    CHECK(tSim.tState.tDash.bRunning);
}

TEST_CASE("PlayerMotor: 달리기 - 방향키 없이 Shift만 누르면 달리기 아님, 게이지 유지")
{
    Sim tSim = MakeStanding();
    for (int i = 0; i < 30; ++i)
        tSim.Tick(kRunOnly);
    CHECK_FALSE(tSim.tState.tDash.bRunning);
    CHECK(tSim.tState.tDash.fGauge == 1.0f);
}

TEST_CASE("PlayerMotor: 달리기 - 공중에서도 적용")
{
    Sim tSim = MakeFalling();
    HoldRun(tSim, kLongHoldTicks);
    CHECK_FALSE(tSim.tBody.bGrounded);
    CHECK(tSim.tState.tDash.bRunning);
}

TEST_CASE("PlayerMotor: 달리기 - 달리면서 점프하면 더 높이")
{
    const float fWalkHeight = MeasureJumpHeight(kWalkRightPress, kWalkRightHold);

    Sim tSim = MakeStanding();
    HoldRun(tSim, kLongHoldTicks + 2);
    REQUIRE(tSim.tState.tDash.bRunning);
    const float fStartY = tSim.tBody.vPos.fY;
    float fMinY = fStartY;
    tSim.Tick(kRunRightJumpPress);
    for (int i = 0; i < 90; ++i)
    {
        fMinY = std::min(fMinY, tSim.tBody.vPos.fY);
        tSim.Tick(kRunRightJumpHold);
    }
    CHECK(fStartY - fMinY > fWalkHeight * 1.2f);  // 속도 1.15배 -> 높이 약 1.32배
}

TEST_CASE("PlayerMotor: 대시 - 짧게 눌렀다 떼면 빠르게 전진, 게이지 절반, 무적")
{
    Sim tSim = MakeStanding();
    tSim.Tick(kIdle);
    const float fStartX = tSim.tBody.vPos.fX;

    tSim.Tick({.iMoveDir = 1, .bDashPressed = true, .bDashHeld = true});
    CHECK_FALSE(tSim.tState.tDash.IsDashing());  // 누르는 중엔 아직 (떼야 나감)
    tSim.Tick({.iMoveDir = 1});                  // 뗌
    CHECK(tSim.tState.tDash.IsDashing());
    CHECK(tSim.tBody.vVel.fX == doctest::Approx(kPlayerDef.fDashSpeed));
    CHECK(tSim.tState.tDash.fGauge == doctest::Approx(1.0f - kPlayerDef.fDashCost));
    CHECK(tSim.tState.tDash.fInvulnTimer > 0.0f);

    const float fDashStartX = tSim.tBody.vPos.fX - kPlayerDef.fDashSpeed * kDt;
    for (int i = 0; i < 30 && tSim.tState.tDash.IsDashing(); ++i)
        tSim.Tick(kIdle);
    CHECK_FALSE(tSim.tState.tDash.IsDashing());
    CHECK(tSim.tState.tDash.fInvulnTimer == 0.0f);

    const float fDistance = tSim.tBody.vPos.fX - fDashStartX;  // 이론 900 * 0.14 = 126 (+ 끝난 뒤 미끄러짐)
    CHECK(fDistance > 110.0f);
    CHECK(fDistance < 170.0f);
    CHECK(fStartX < fDashStartX);
}

TEST_CASE("PlayerMotor: 대시 - 두 고정 틱 사이에 눌렀다 뗀 아주 짧은 탭도 대시")
{
    Sim tSim = MakeStanding();
    tSim.Tick({.iMoveDir = 1, .bDashPressed = true, .bDashHeld = false});
    CHECK(tSim.tState.tDash.IsDashing());
}

TEST_CASE("PlayerMotor: 대시 - 방향키 없으면 마지막으로 누른 방향으로")
{
    Sim tSim = MakeStanding();
    tSim.Tick(kWalkLeft);
    tSim.Tick(kIdle);
    TapDash(tSim, 0);
    CHECK(tSim.tState.tDash.iDashDir == -1);
    CHECK(tSim.tBody.vVel.fX < 0.0f);
}

TEST_CASE("PlayerMotor: 대시 - 위아래로는 안 움직임 (공중에서도 높이 유지)")
{
    Sim tSim = MakeFalling();
    tSim.Tick(kIdle);
    TapDash(tSim, 1);
    REQUIRE(tSim.tState.tDash.IsDashing());
    const float fY = tSim.tBody.vPos.fY;
    for (int i = 0; i < 5; ++i)
        tSim.Tick(kIdle);
    REQUIRE(tSim.tState.tDash.IsDashing());
    CHECK(tSim.tBody.vPos.fY == doctest::Approx(fY));
    CHECK(tSim.tBody.vVel.fY == 0.0f);
}

TEST_CASE("PlayerMotor: 대시 - 점프해서 올라가는 중에도 나감 (Z 누른 채)")
{
    Sim tSim = MakeStanding();
    tSim.Tick(kIdle);
    tSim.Tick({.iMoveDir = 1, .bJumpPressed = true, .bJumpHeld = true});
    for (int i = 0; i < 5; ++i)
        tSim.Tick({.iMoveDir = 1, .bJumpHeld = true});
    REQUIRE_FALSE(tSim.tBody.bGrounded);

    tSim.Tick({.iMoveDir = 1, .bJumpHeld = true, .bDashPressed = true, .bDashHeld = true});
    tSim.Tick({.iMoveDir = 1, .bJumpHeld = true});
    CHECK(tSim.tState.tDash.IsDashing());
    CHECK(tSim.tBody.vVel.fX == doctest::Approx(kPlayerDef.fDashSpeed));
}

TEST_CASE("PlayerMotor: 대시 - 달리다가 점프한 공중에서 다시 탭하면 나감")
{
    Sim tSim = MakeStanding();
    HoldRun(tSim, kLongHoldTicks + 5);
    tSim.Tick({.iMoveDir = 1, .bJumpPressed = true, .bJumpHeld = true, .bDashHeld = true});
    for (int i = 0; i < 4; ++i)
        tSim.Tick({.iMoveDir = 1, .bJumpHeld = true, .bDashHeld = true});
    tSim.Tick({.iMoveDir = 1, .bJumpHeld = true});  // Shift 뗌 (길게 눌렀으니 대시 X)
    REQUIRE_FALSE(tSim.tState.tDash.IsDashing());
    REQUIRE_FALSE(tSim.tBody.bGrounded);

    tSim.Tick({.iMoveDir = 1, .bJumpHeld = true, .bDashPressed = true, .bDashHeld = true});
    tSim.Tick({.iMoveDir = 1, .bJumpHeld = true});
    CHECK(tSim.tState.tDash.IsDashing());
}

TEST_CASE("PlayerMotor: 대시 - 게이지가 절반 미만이면 안 나감")
{
    Sim tSim = MakeStanding();
    tSim.tState.tDash.fGauge = kPlayerDef.fDashCost - 0.05f;
    TapDash(tSim, 1);
    CHECK_FALSE(tSim.tState.tDash.IsDashing());
}


TEST_CASE("PlayerMotor: 장거리는 달리기가 대시보다 멀리 (같은 게이지 0.5)")
{
    const float fDashDistance = kPlayerDef.fDashSpeed * kPlayerDef.fDashTime;
    const float fRunDistance =
        kPlayerDef.fMoveSpeed * kPlayerDef.fRunSpeedMultiplier * (kPlayerDef.fDashCost / kPlayerDef.fRunDrainPerSec);
    CHECK(fRunDistance > fDashDistance);
}

TEST_CASE("PlayerMotor: facing - 마지막 이동 방향 유지")
{
    Sim tSim = MakeStanding();
    CHECK(tSim.tState.iFacing == 1);
    tSim.Tick(kWalkLeft);
    CHECK(tSim.tState.iFacing == -1);
    tSim.Tick(kIdle);
    CHECK(tSim.tState.iFacing == -1);
}

TEST_CASE("PlayerMotor: 가변 점프 - 바로 떼면 꾹 누를 때보다 낮음")
{
    const float fFullHeight = MeasureJumpHeight(kPress, kHold);
    const float fTapHeight = MeasureJumpHeight(kTap, kIdle);

    CHECK(fFullHeight > 100.0f);  // 이론값 v² / 2g = 120px
    CHECK(fTapHeight > 0.0f);
    CHECK(fTapHeight < fFullHeight * 0.5f);
}
