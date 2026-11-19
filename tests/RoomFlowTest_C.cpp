// 방 진행 테스트 (docs/design/room-flow-ui.md 4장)
#include "core/EntitySide_C.h"
#include "core/FixedTimestep_C.h"
#include "core/World_U.h"
#include "data/PlayerDef_C.h"
#include "data/Rooms_C.h"
#include "systems/RoomFlow_C.h"

#include <doctest/doctest.h>

namespace
{
constexpr float kDt = FixedTimestep::kStep;
constexpr MenuIntent kNoMenu = {};
constexpr MenuIntent kConfirm = {.bConfirmPressed = true};

int CountEnemies(const World& tWorld)
{
    int iCount = 0;
    tWorld.ForEachAlive([&](Handle, const Entity& tEntity) {
        if (IsEnemySide(tEntity.eType))
            ++iCount;
    });
    return iCount;
}

void KillAllEnemies(World& tWorld)
{
    tWorld.ForEachAlive([&](Handle hEntity, Entity& tEntity) {
        if (IsEnemySide(tEntity.eType))
            tWorld.Destroy(hEntity);
    });
    tWorld.FlushRemovals();
}

void MovePlayerToDoor(World& tWorld, Handle hPlayer, const RoomDef& tDef)
{
    const Rect rcDoor = GetRoomDoor(tDef);
    tWorld.Get(hPlayer)->tBody.vPos = {rcDoor.fX + 4.0f, rcDoor.fY + 4.0f};
}

struct Session
{
    Tilemap tRoom;
    RoomFlow tFlow{tRoom};
    World tWorld;
    Handle hPlayer;

    void Tick(const MenuIntent& tMenu = kNoMenu) { tFlow.Update(tWorld, hPlayer, tMenu, kDt); }
};
} // namespace

TEST_CASE("RoomFlow: 타이틀에서 Enter -> 튜토리얼, 플레이어 생성")
{
    Session tS;
    CHECK(tS.tFlow.GetPhase() == GamePhase::Title);
    tS.Tick(kConfirm);
    CHECK(tS.tFlow.GetPhase() == GamePhase::Playing);
    CHECK(tS.tFlow.GetRoomIndex() == 0);
    REQUIRE(tS.tWorld.Get(tS.hPlayer) != nullptr);
    CHECK(tS.tRoom.GetCols() == kRooms[0].iCols);
}

TEST_CASE("RoomFlow: 적이 남으면 Locked, 전멸하면 Cleared")
{
    Session tS;
    tS.Tick(kConfirm);
    REQUIRE(CountEnemies(tS.tWorld) > 0);
    tS.Tick();
    CHECK(tS.tFlow.GetRoomState() == RoomState::Locked);

    KillAllEnemies(tS.tWorld);
    tS.Tick();
    CHECK(tS.tFlow.GetRoomState() == RoomState::Cleared);
}

TEST_CASE("RoomFlow: 잠긴 문은 안 열림, 열린 문에 닿으면 다음 방 (HP 유지, 이전 적 없음)")
{
    Session tS;
    tS.Tick(kConfirm);
    MovePlayerToDoor(tS.tWorld, tS.hPlayer, kRooms[0]);
    tS.Tick();
    CHECK(tS.tFlow.GetRoomIndex() == 0);  // 아직 Locked

    tS.tWorld.Get(tS.hPlayer)->tBody.vPos = GetRoomPlayerSpawn(kRooms[0]);  // 문에서 떨어뜨려 둠
    tS.tWorld.Get(tS.hPlayer)->fHp = 42.0f;
    KillAllEnemies(tS.tWorld);
    tS.Tick();  // Cleared
    MovePlayerToDoor(tS.tWorld, tS.hPlayer, kRooms[0]);
    tS.Tick();
    CHECK(tS.tFlow.GetRoomIndex() == 1);
    CHECK(tS.tFlow.GetRoomState() == RoomState::Locked);
    CHECK(tS.tRoom.GetCols() == kRooms[1].iCols);

    const Entity* pPlayer = tS.tWorld.Get(tS.hPlayer);
    REQUIRE(pPlayer != nullptr);
    CHECK(pPlayer->fHp == 42.0f);
    CHECK(pPlayer->tBody.vPos.fX == doctest::Approx(GetRoomPlayerSpawn(kRooms[1]).fX));
    CHECK(CountEnemies(tS.tWorld) == static_cast<int>(kRooms[1].spEnemies.size()));
}

TEST_CASE("RoomFlow: 플레이어 HP 0 -> GameOver -> Enter -> 튜토리얼부터, HP 풀")
{
    Session tS;
    tS.Tick(kConfirm);
    tS.tWorld.Get(tS.hPlayer)->fHp = 0.0f;
    tS.Tick();
    CHECK(tS.tFlow.GetPhase() == GamePhase::GameOver);

    const Handle hOld = tS.hPlayer;
    tS.Tick(kConfirm);
    CHECK(tS.tFlow.GetPhase() == GamePhase::Playing);
    CHECK(tS.tFlow.GetRoomIndex() == 0);
    CHECK(tS.tWorld.Get(hOld) == nullptr);  // 옛 플레이어는 지워짐
    REQUIRE(tS.tWorld.Get(tS.hPlayer) != nullptr);
    CHECK(tS.tWorld.Get(tS.hPlayer)->fHp == kPlayerDef.fMaxHp);
}

TEST_CASE("RoomFlow: 마지막(보스) 방 클리어 -> Victory")
{
    Session tS;
    tS.Tick(kConfirm);
    for (int iRoom = 0; iRoom + 1 < static_cast<int>(kRooms.size()); ++iRoom)
    {
        KillAllEnemies(tS.tWorld);
        tS.Tick();
        MovePlayerToDoor(tS.tWorld, tS.hPlayer, kRooms[static_cast<std::size_t>(iRoom)]);
        tS.Tick();
    }
    REQUIRE(tS.tFlow.GetRoomDef().bBossRoom);
    CHECK(tS.tFlow.GetPhase() == GamePhase::Playing);

    KillAllEnemies(tS.tWorld);
    tS.Tick();
    CHECK(tS.tFlow.GetPhase() == GamePhase::Victory);
}
