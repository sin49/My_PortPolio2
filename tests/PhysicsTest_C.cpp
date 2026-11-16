#include "core/Body_C.h"
#include "core/FixedTimestep_C.h"
#include "core/Tilemap_C.h"
#include "systems/Physics_C.h"

#include <array>
#include <string_view>

#include <doctest/doctest.h>

namespace
{
constexpr float kDt = FixedTimestep::kStep;

// 10 x 6: 위 천장(행 0), 아래 바닥(행 5, 윗면 y=160), 열 7에 벽(행 3~4, 왼쪽면 x=224)
constexpr std::array<std::string_view, 6> kBox = {
    "##########",
    "..........",
    "..........",
    ".......#..",
    ".......#..",
    "##########",
};

Tilemap MakeBox()
{
    return *Tilemap::FromRows(kBox);
}

Body MakeBody(float fX, float fY)
{
    return Body{.vPos = {fX, fY}, .vSize = {24.0f, 40.0f}};
}
} // namespace

TEST_CASE("Physics: 떨어지면 바닥에 착지하고 grounded")
{
    const Tilemap tMap = MakeBox();
    const Physics tPhysics;
    Body tBody = MakeBody(32.0f, 40.0f);

    for (int i = 0; i < 60; ++i)
        tPhysics.Update(tBody, tMap, kDt);

    CHECK(tBody.bGrounded);
    CHECK(tBody.vVel.fY == 0.0f);
    CHECK(tBody.vPos.fY == doctest::Approx(160.0f - 40.0f));
}

TEST_CASE("Physics: 벽으로 걸어가면 벽 앞에서 멈춤")
{
    const Tilemap tMap = MakeBox();
    const Physics tPhysics;
    Body tBody = MakeBody(100.0f, 120.0f);

    for (int i = 0; i < 60; ++i)
    {
        tBody.vVel.fX = 300.0f;  // Motor가 매 틱 속도를 정하는 상황 흉내
        tPhysics.Update(tBody, tMap, kDt);
    }

    CHECK(tBody.vVel.fX == 0.0f);
    CHECK(tBody.vPos.fX + tBody.vSize.fX == doctest::Approx(224.0f));
    CHECK(tBody.bGrounded);
}

TEST_CASE("Physics: 천장에 박으면 vVel.fY = 0")
{
    const Tilemap tMap = MakeBox();
    const Physics tPhysics;
    Body tBody = MakeBody(32.0f, 40.0f);
    tBody.vVel.fY = -600.0f;

    tPhysics.Update(tBody, tMap, kDt);

    CHECK(tBody.vVel.fY == 0.0f);
    CHECK(tBody.vPos.fY == doctest::Approx(32.0f));
    CHECK_FALSE(tBody.bGrounded);
}

TEST_CASE("Physics: 벽에 딱 붙어 있어도 옆 타일 오판 없음")
{
    const Tilemap tMap = MakeBox();
    const Physics tPhysics;
    Body tBody = MakeBody(224.0f - 24.0f, 120.0f);  // 오른쪽 면 = 벽 왼쪽 면

    for (int i = 0; i < 10; ++i)
        tPhysics.Update(tBody, tMap, kDt);
    CHECK(tBody.vPos.fX == 200.0f);  // 서 있기만 하면 밀려나지 않음
    CHECK(tBody.bGrounded);

    tBody.vVel.fX = -60.0f;  // 벽에서 멀어지는 방향은 막히지 않음
    tPhysics.Update(tBody, tMap, kDt);
    CHECK(tBody.vPos.fX == doctest::Approx(199.0f));
}
