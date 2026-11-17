#include "systems/RoomFlow_C.h"

#include "core/World_U.h"
#include "data/EntityFactory_C.h"
#include "data/Rooms_C.h"

#include <cstddef>

namespace
{
const RoomDef& RoomAt(int iIndex)
{
    return kRooms[static_cast<std::size_t>(iIndex)];
}

Tilemap BuildTiles(const RoomDef& tDef)
{
    return Tilemap::FromSolids(tDef.iCols, tDef.iRows, tDef.spSolids);
}

// 방 클리어 조건: 적 진영 엔티티가 하나도 없음 (투사체·드랍 제외)
bool HasEnemiesLeft(const World& tWorld)
{
    bool bAny = false;
    tWorld.ForEachAlive([&](Handle, const Entity& tEntity) {
        if (tEntity.eType == EntityType::Enemy && tEntity.fHp > 0.0f)  // TODO: Boss, Shooter 추가 후 포함
            bAny = true;
    });
    return bAny;
}

// 플레이어(hKeep) 외 전부 삭제 예약 후 바로 Flush (방 전환 시점은 순회 밖이라 안전)
void ClearWorld(World& tWorld, Handle hKeep)
{
    tWorld.ForEachAlive([&](Handle hEntity, Entity&) {
        if (!(hEntity == hKeep))
            tWorld.Destroy(hEntity);
    });
    tWorld.FlushRemovals();
}
} // namespace

RoomFlow::RoomFlow(Tilemap& tRoom)
    : m_tRoom(tRoom)
{
    m_tRoom = BuildTiles(RoomAt(0));
}

const RoomDef& RoomFlow::GetRoomDef() const
{
    return RoomAt(m_iRoomIndex);
}

int RoomFlow::GetRoomEnemyTotal() const
{
    return static_cast<int>(GetRoomDef().spEnemies.size());
}

void RoomFlow::StartNewGame(World& tWorld, Handle& hPlayer)
{
    ClearWorld(tWorld, Handle{});  // 플레이어까지 전부
    hPlayer = tWorld.Create(MakePlayerEntity(GetRoomPlayerSpawn(RoomAt(0))));
    m_fPlayTime = 0.0f;
    LoadRoom(tWorld, hPlayer, 0);
    m_ePhase = GamePhase::Playing;
}

void RoomFlow::PrepareTitle(World& tWorld, Handle& hPlayer)
{
    StartNewGame(tWorld, hPlayer);
    m_ePhase = GamePhase::Title;
}

void RoomFlow::LoadRoom(World& tWorld, Handle hPlayer, int iRoomIndex)
{
    ClearWorld(tWorld, hPlayer);

    m_iRoomIndex = iRoomIndex;
    const RoomDef& tDef = RoomAt(iRoomIndex);
    m_tRoom = BuildTiles(tDef);

    if (Entity* pPlayer = tWorld.Get(hPlayer))  // HP·상태 유지, 위치만 스폰으로
    {
        pPlayer->tBody.vPos = GetRoomPlayerSpawn(tDef);
        pPlayer->tBody.vVel = {};
        pPlayer->tMotor.iFacing = 1;
    }

    const float fFloorTop = GetRoomFloorTop(tDef);
    for (const EnemySpawn& tSpawn : tDef.spEnemies)
        tWorld.Create(MakeEnemyEntity(tSpawn, {tSpawn.fTileX * kTileSize, fFloorTop}));

    m_eRoomState = tDef.spEnemies.empty() ? RoomState::Cleared : RoomState::Locked;
}

void RoomFlow::Update(World& tWorld, Handle& hPlayer, const MenuIntent& tMenu, float fDt)
{
    if (m_ePhase != GamePhase::Playing)
    {
        if (tMenu.bConfirmPressed)
            StartNewGame(tWorld, hPlayer);
        return;
    }

    m_fPlayTime += fDt;

    const Entity* pPlayer = tWorld.Get(hPlayer);
    if (pPlayer == nullptr || pPlayer->fHp <= 0.0f)
    {
        m_ePhase = GamePhase::GameOver;
        return;
    }

    if (m_eRoomState == RoomState::Locked && !HasEnemiesLeft(tWorld))
    {
        m_eRoomState = RoomState::Cleared;
        if (GetRoomDef().bBossRoom)
        {
            m_ePhase = GamePhase::Victory;
            return;
        }
    }

    const bool bHasNextRoom = m_iRoomIndex + 1 < static_cast<int>(kRooms.size());
    if (m_eRoomState == RoomState::Cleared && bHasNextRoom &&
        Overlaps(GetBounds(pPlayer->tBody), GetRoomDoor(GetRoomDef())))
        LoadRoom(tWorld, hPlayer, m_iRoomIndex + 1);
}
