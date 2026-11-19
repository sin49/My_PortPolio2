// 스트레스 소환 테스트 (docs/design/stress-sound.md 1장)
#include "core/World_U.h"
#include "data/EntityFactory_C.h"
#include "data/ProjectileDef_C.h"
#include "systems/Items_C.h"
#include "systems/Projectile_C.h"
#include "systems/StressSpawner_C.h"

#include <doctest/doctest.h>

#include <array>
#include <cmath>

namespace
{
struct Bench
{
    static constexpr std::array<TileRect, 2> kSolids = {{{10, 15, 8, 1}, {25, 10, 2, 9}}};
    Tilemap tRoom = Tilemap::FromSolids(40, 22, kSolids);
    World tWorld;
    Handle hPlayer = tWorld.Create(MakePlayerEntity({100.0f, 600.0f}));
};

bool TouchesSolid(const Tilemap& tRoom, const Body& tBody)
{
    for (int iY = static_cast<int>(tBody.vPos.fY / kTileSize);
         iY <= static_cast<int>((tBody.vPos.fY + tBody.vSize.fY) / kTileSize); ++iY)
        for (int iX = static_cast<int>(tBody.vPos.fX / kTileSize);
             iX <= static_cast<int>((tBody.vPos.fX + tBody.vSize.fX) / kTileSize); ++iX)
            if (tRoom.IsSolid(iX, iY))
                return true;
    return false;
}
} // namespace

TEST_CASE("StressSpawner: 작은 Chaser만, 벽과 안 겹치고 플레이어 근처는 제외")
{
    Bench tB;
    StressSpawner tSpawner;
    for (int i = 0; i < 100; ++i)
        REQUIRE(tSpawner.SpawnSmallChaser(tB.tWorld, tB.tRoom, tB.hPlayer));

    const Body& tPlayer = tB.tWorld.Get(tB.hPlayer)->tBody;
    int iEnemies = 0;
    tB.tWorld.ForEachAlive([&](Handle hEntity, const Entity& tEntity) {
        if (hEntity == tB.hPlayer)
            return;
        ++iEnemies;
        CHECK(tEntity.eType == EntityType::Enemy);
        CHECK(tEntity.fHp == doctest::Approx(GetEnemySizeDef(EnemySize::Small).fMaxHp));
        CHECK_FALSE(tEntity.tAI.bPassive);
        CHECK_FALSE(TouchesSolid(tB.tRoom, tEntity.tBody));
        const float fDx = (tEntity.tBody.vPos.fX + tEntity.tBody.vSize.fX * 0.5f) - (tPlayer.vPos.fX + tPlayer.vSize.fX * 0.5f);
        const float fDy = (tEntity.tBody.vPos.fY + tEntity.tBody.vSize.fY * 0.5f) - (tPlayer.vPos.fY + tPlayer.vSize.fY * 0.5f);
        CHECK(std::sqrt(fDx * fDx + fDy * fDy) >= kStressSpawnDef.fMinPlayerDistance);
    });
    CHECK(iEnemies == 100);
}

TEST_CASE("StressSpawner: 제한 없음 = World 풀이 꽉 찰 때까지, 꽉 차면 false (throw 없음)")
{
    Bench tB;
    StressSpawner tSpawner;
    int iSpawned = 0;
    for (int i = 0; i < static_cast<int>(World::kCapacity) * 2; ++i)  // 풀보다 많이 눌러도
        if (tSpawner.SpawnSmallChaser(tB.tWorld, tB.tRoom, tB.hPlayer))
            ++iSpawned;
    CHECK(tB.tWorld.GetAliveCount() == static_cast<int>(World::kCapacity));
    CHECK(iSpawned == static_cast<int>(World::kCapacity) - 1);  // 플레이어 1칸 뺀 전부
}

TEST_CASE("풀이 꽉 차도 안 죽음: 투사체·드랍 생성은 건너뜀 (무효 핸들)")
{
    Bench tB;
    StressSpawner tSpawner;
    while (tSpawner.SpawnSmallChaser(tB.tWorld, tB.tRoom, tB.hPlayer))
    {
    }
    REQUIRE(tB.tWorld.GetAliveCount() == static_cast<int>(World::kCapacity));

    Projectiles tProjectiles;
    Handle hBullet;
    CHECK_NOTHROW(hBullet = tProjectiles.Spawn(tB.tWorld, {300.0f, 300.0f}, {1.0f, 0.0f}, kBulletDef, Faction::Enemy));
    CHECK(tB.tWorld.Get(hBullet) == nullptr);

    Items tItems;
    Handle hDrop;
    CHECK_NOTHROW(hDrop = tItems.SpawnDrop(tB.tWorld, {300.0f, 300.0f}, ItemId::Heart));
    CHECK(tB.tWorld.Get(hDrop) == nullptr);
    CHECK(tItems.FindDrop(hDrop) == nullptr);
}
