#include "core/Tilemap_C.h"
#include "data/PlayerDef_C.h"
#include "data/TestRoom_C.h"

#include <array>
#include <cmath>
#include <string_view>

#include <doctest/doctest.h>

TEST_CASE("Tilemap: 테스트 방은 40 x 22로 파싱됨")
{
    const auto optRoom = Tilemap::FromRows(kTestRoom);
    REQUIRE(optRoom.has_value());
    CHECK(optRoom->GetCols() == kRoomCols);
    CHECK(optRoom->GetRows() == kRoomRows);
}

TEST_CASE("Tilemap: 플레이어 스폰 위치는 빈칸")
{
    const auto optRoom = Tilemap::FromRows(kTestRoom);
    REQUIRE(optRoom.has_value());

    const Vec2 vSpawn = kTestRoomPlayerSpawn;
    const Vec2 vSize = kPlayerDef.vSize;
    const int iLeft = static_cast<int>(std::floor(vSpawn.fX / kTileSize));
    const int iRight = static_cast<int>(std::floor((vSpawn.fX + vSize.fX - 0.001f) / kTileSize));
    const int iTop = static_cast<int>(std::floor(vSpawn.fY / kTileSize));
    const int iBottom = static_cast<int>(std::floor((vSpawn.fY + vSize.fY - 0.001f) / kTileSize));
    for (int iTileY = iTop; iTileY <= iBottom; ++iTileY)
        for (int iTileX = iLeft; iTileX <= iRight; ++iTileX)
            CHECK_FALSE(optRoom->IsSolid(iTileX, iTileY));
}

TEST_CASE("Tilemap: 문자 해석과 범위 밖 처리")
{
    constexpr std::array<std::string_view, 2> kRows = {"#.", ".#"};
    const auto optMap = Tilemap::FromRows(kRows);
    REQUIRE(optMap.has_value());

    CHECK(optMap->IsSolid(0, 0));
    CHECK_FALSE(optMap->IsSolid(1, 0));
    CHECK_FALSE(optMap->IsSolid(0, 1));
    CHECK(optMap->IsSolid(1, 1));

    // 범위 밖은 벽
    CHECK(optMap->IsSolid(-1, 0));
    CHECK(optMap->IsSolid(0, -1));
    CHECK(optMap->IsSolid(2, 0));
    CHECK(optMap->IsSolid(0, 2));
}

TEST_CASE("Tilemap: 잘못된 데이터는 nullopt")
{
    constexpr std::array<std::string_view, 2> kRagged = {"###", "##"};
    CHECK_FALSE(Tilemap::FromRows(kRagged).has_value());

    constexpr std::array<std::string_view, 1> kUnknownChar = {"#x#"};
    CHECK_FALSE(Tilemap::FromRows(kUnknownChar).has_value());

    CHECK_FALSE(Tilemap::FromRows({}).has_value());
}
