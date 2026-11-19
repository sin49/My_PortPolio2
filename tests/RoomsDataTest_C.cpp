// 방 데이터 검사 (Rooms_C.h): 손으로 적은 좌표가 틀리면 적이 벽에 박히거나 공중에서 떨어짐 -> 테스트로 막음
#include "core/Tilemap_C.h"
#include "data/EntityFactory_C.h"
#include "data/Rooms_C.h"

#include <doctest/doctest.h>

#include <cmath>
#include <ostream>
#include <string>

namespace
{
bool TouchesSolid(const Tilemap& tMap, const Rect& rcArea)
{
    for (int iY = static_cast<int>(std::floor(rcArea.fY / kTileSize));
         iY <= static_cast<int>(std::floor((rcArea.fY + rcArea.fH - 0.01f) / kTileSize)); ++iY)
        for (int iX = static_cast<int>(std::floor(rcArea.fX / kTileSize));
             iX <= static_cast<int>(std::floor((rcArea.fX + rcArea.fW - 0.01f) / kTileSize)); ++iX)
            if (tMap.IsSolid(iX, iY))
                return true;
    return false;
}
} // namespace

TEST_CASE("Rooms: 모든 적은 벽과 안 겹치고, 발 바로 아래가 바닥·발판 (공중 생성 X)")
{
    for (const RoomDef& tRoom : kRooms)
    {
        const Tilemap tMap = Tilemap::FromSolids(tRoom.iCols, tRoom.iRows, tRoom.spSolids);
        for (const EnemySpawn& tSpawn : tRoom.spEnemies)
        {
            const Entity tEnemy = MakeEnemyEntity(tSpawn, GetSpawnFeet(tRoom, tSpawn));
            const Rect rcBody = GetBounds(tEnemy.tBody);
            CAPTURE(std::string(tRoom.svName));
            CAPTURE(tSpawn.fTileX);
            CAPTURE(tSpawn.iStandTileY);
            CHECK_FALSE(TouchesSolid(tMap, rcBody));
            // 발 아래 1px 줄: 몸 폭 중 가운데가 단단해야 (가장자리에 걸치는 건 허용하지 않음)
            const int iBelowY = static_cast<int>(std::floor((rcBody.fY + rcBody.fH + 1.0f) / kTileSize));
            const int iCenterX = static_cast<int>(std::floor((rcBody.fX + rcBody.fW * 0.5f) / kTileSize));
            CHECK(tMap.IsSolid(iCenterX, iBelowY));
        }
    }
}

TEST_CASE("Rooms: 플레이어 시작 자리·문은 벽에 막히지 않음")
{
    for (const RoomDef& tRoom : kRooms)
    {
        CAPTURE(std::string(tRoom.svName));
        const Tilemap tMap = Tilemap::FromSolids(tRoom.iCols, tRoom.iRows, tRoom.spSolids);
        const Entity tPlayer = MakePlayerEntity(GetRoomPlayerSpawn(tRoom));
        CHECK_FALSE(TouchesSolid(tMap, GetBounds(tPlayer.tBody)));
        if (!tRoom.bBossRoom)
            CHECK_FALSE(TouchesSolid(tMap, GetRoomDoor(tRoom)));
    }
}
