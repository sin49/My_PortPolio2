// 투사체 + Shooter 테스트 (docs/design/boss-projectile.md 5장, shooter.md)
#include "core/EntitySide_C.h"
#include "core/FixedTimestep_C.h"
#include "core/Tilemap_C.h"
#include "core/World_U.h"
#include "data/EnemyDef_C.h"
#include "data/EntityFactory_C.h"
#include "data/ProjectileDef_C.h"
#include "systems/Combat_C.h"
#include "systems/Projectile_C.h"
#include "systems/ShooterAI_C.h"
#include "systems/SpatialQuery_C.h"

#include <span>
#include <vector>

#include <doctest/doctest.h>

namespace
{
constexpr float kDt = FixedTimestep::kStep;

// 40 x 20칸 빈 방 (바깥 테두리만 벽)
Tilemap MakeEmptyRoom()
{
    return Tilemap::FromSolids(40, 20, std::span<const TileRect>{});
}

struct Arena
{
    Tilemap tRoom = MakeEmptyRoom();
    World tWorld;
    SpatialQuery tSpatial{tWorld};
    Projectiles tProjectiles;
    Combat tCombat;
    std::vector<Vec2> vecWallHits;
    std::vector<HitReport> vecReports;

    // Game.FixedUpdate의 투사체 부분과 같은 순서
    void Tick()
    {
        tProjectiles.FireFromIntents(tWorld, kBulletDef);
        vecWallHits.clear();
        tProjectiles.Update(tWorld, tRoom, kDt, vecWallHits);
        tSpatial.Rebuild();
        vecReports.clear();
        tCombat.ResolveProjectileHits(tWorld, tSpatial, vecReports);
        tWorld.FlushRemovals();
    }

    int CountBullets() const
    {
        int iCount = 0;
        tWorld.ForEachAlive([&](Handle, const Entity& tEntity) {
            if (tEntity.eType == EntityType::Projectile)
                ++iCount;
        });
        return iCount;
    }
};
} // namespace

TEST_CASE("Projectile: 직진 (중력 없음)")
{
    Arena tArena;
    const Handle hBullet =
        tArena.tProjectiles.Spawn(tArena.tWorld, {200.0f, 200.0f}, {1.0f, 0.0f}, kBulletDef, Faction::Enemy);
    const float fStartX = tArena.tWorld.Get(hBullet)->tBody.vPos.fX;
    for (int i = 0; i < 30; ++i)
        tArena.Tick();
    const Entity* pBullet = tArena.tWorld.Get(hBullet);
    REQUIRE(pBullet != nullptr);
    CHECK(pBullet->tBody.vPos.fX == doctest::Approx(fStartX + kBulletDef.fSpeed * 0.5f).epsilon(0.01));
    CHECK(pBullet->tBody.vPos.fY == doctest::Approx(200.0f - kBulletDef.vSize.fY * 0.5f));
}

TEST_CASE("Projectile: 수명이 끝나면 삭제")
{
    Arena tArena;
    ProjectileDef tSlow = kBulletDef;
    tSlow.fSpeed = 1.0f;
    tSlow.fLifetime = 0.5f;
    const Handle hBullet =
        tArena.tProjectiles.Spawn(tArena.tWorld, {200.0f, 200.0f}, {1.0f, 0.0f}, tSlow, Faction::Enemy);
    for (int i = 0; i < 25; ++i)
        tArena.Tick();
    CHECK(tArena.tWorld.Get(hBullet) != nullptr);
    for (int i = 0; i < 10; ++i)
        tArena.Tick();
    CHECK(tArena.tWorld.Get(hBullet) == nullptr);
}

TEST_CASE("Projectile: 벽 타일에 닿으면 삭제 + 벽 위치 보고")
{
    Arena tArena;
    tArena.tProjectiles.Spawn(tArena.tWorld, {100.0f, 200.0f}, {-1.0f, 0.0f}, kBulletDef, Faction::Enemy);  // 왼쪽 벽으로
    bool bWallHit = false;
    for (int i = 0; i < 60 && tArena.CountBullets() > 0; ++i)
    {
        tArena.Tick();
        bWallHit = bWallHit || !tArena.vecWallHits.empty();
    }
    CHECK(tArena.CountBullets() == 0);
    CHECK(bWallHit);
}

TEST_CASE("Projectile: 적 탄은 플레이어만 맞히고 적은 지나감, 맞으면 탄 삭제 (관통 X)")
{
    Arena tArena;
    const EnemySpawn tSpawn = {.eKind = SpawnKind::Chaser, .fTileX = 0.0f, .eSize = EnemySize::Large};
    const Handle hChaser = tArena.tWorld.Create(MakeEnemyEntity(tSpawn, {260.0f, 260.0f}));
    const Handle hPlayer = tArena.tWorld.Create(MakePlayerEntity({340.0f, 220.0f}));
    const float fChaserHp = tArena.tWorld.Get(hChaser)->fHp;

    // 왼쪽에서 오른쪽으로: 적(260 부근)을 지나 플레이어(340~364)에 맞음
    tArena.tProjectiles.Spawn(tArena.tWorld, {200.0f, 240.0f}, {1.0f, 0.0f}, kBulletDef, Faction::Enemy);
    int iHits = 0;
    for (int i = 0; i < 60; ++i)
    {
        tArena.Tick();
        iHits += static_cast<int>(tArena.vecReports.size());
    }
    CHECK(tArena.tWorld.Get(hChaser)->fHp == doctest::Approx(fChaserHp));
    CHECK(tArena.tWorld.Get(hPlayer)->fHp == doctest::Approx(kPlayerDef.fMaxHp - kBulletDef.fDamage));
    CHECK(iHits == 1);
    CHECK(tArena.CountBullets() == 0);
}

TEST_CASE("Projectile: 대시 무적이면 탄이 그냥 지나감")
{
    Arena tArena;
    Entity tPlayer = MakePlayerEntity({300.0f, 220.0f});
    tPlayer.tMotor.tDash.fInvulnTimer = 10.0f;
    const Handle hPlayer = tArena.tWorld.Create(tPlayer);
    tArena.tProjectiles.Spawn(tArena.tWorld, {200.0f, 240.0f}, {1.0f, 0.0f}, kBulletDef, Faction::Enemy);
    for (int i = 0; i < 30; ++i)
        tArena.Tick();
    CHECK(tArena.tWorld.Get(hPlayer)->fHp == doctest::Approx(kPlayerDef.fMaxHp));
    CHECK(tArena.CountBullets() == 1);
}

TEST_CASE("Combat: 디버그 무적(F1)이면 플레이어는 탄·근접 모두 안 맞음, 적은 그대로 맞음")
{
    Arena tArena;
    tArena.tCombat.SetPlayerInvincible(true);
    const Handle hPlayer = tArena.tWorld.Create(MakePlayerEntity({300.0f, 220.0f}));
    tArena.tProjectiles.Spawn(tArena.tWorld, {200.0f, 240.0f}, {1.0f, 0.0f}, kBulletDef, Faction::Enemy);

    // 근접: 플레이어 몸에 겹친 적 히트박스
    Entity tChaser = MakeEnemyEntity({.eKind = SpawnKind::Chaser}, {330.0f, 260.0f});
    tChaser.tAttack.ePhase = AttackPhase::Active;
    tChaser.tAttack.rcHitbox = GetBounds(tArena.tWorld.Get(hPlayer)->tBody);
    tArena.tWorld.Create(tChaser);

    for (int i = 0; i < 30; ++i)
    {
        tArena.Tick();
        tArena.tSpatial.Rebuild();
        tArena.vecReports.clear();
        tArena.tCombat.ResolveHits(tArena.tWorld, tArena.tSpatial, tArena.vecReports, kDt);
    }
    CHECK(tArena.tWorld.Get(hPlayer)->fHp == doctest::Approx(kPlayerDef.fMaxHp));

    // 끄면 다시 맞음
    tArena.tCombat.SetPlayerInvincible(false);
    tArena.tProjectiles.Spawn(tArena.tWorld, {250.0f, 240.0f}, {1.0f, 0.0f}, kBulletDef, Faction::Enemy);
    for (int i = 0; i < 30; ++i)
        tArena.Tick();
    CHECK(tArena.tWorld.Get(hPlayer)->fHp < kPlayerDef.fMaxHp);
}

TEST_CASE("Projectile: 발사 의도 = 부채꼴 3발 / 원형 8발, 의도는 소비됨")
{
    Arena tArena;
    Entity tShooter = MakeShooterEntity({400.0f, 400.0f});
    tShooter.tAttackIntent = {.bFirePressed = true, .vAim = {1.0f, 0.0f}, .iShotCount = 3, .fSpreadDeg = 24.0f};
    const Handle hShooter = tArena.tWorld.Create(tShooter);
    tArena.tProjectiles.FireFromIntents(tArena.tWorld, kBulletDef);
    CHECK(tArena.CountBullets() == 3);
    CHECK_FALSE(tArena.tWorld.Get(hShooter)->tAttackIntent.bFirePressed);
    tArena.tProjectiles.FireFromIntents(tArena.tWorld, kBulletDef);  // 다시 불러도 안 쏨
    CHECK(tArena.CountBullets() == 3);

    tArena.tWorld.Get(hShooter)->tAttackIntent = {
        .bFirePressed = true, .vAim = {1.0f, 0.0f}, .iShotCount = 8, .fSpreadDeg = 360.0f};
    tArena.tProjectiles.FireFromIntents(tArena.tWorld, kBulletDef);
    CHECK(tArena.CountBullets() == 11);

    // 원형 8발 중 정확히 왼쪽 1발, 위 1발 (부채꼴 3발은 오른쪽 근처)
    int iLeft = 0;
    int iUp = 0;
    tArena.tWorld.ForEachAlive([&](Handle, const Entity& tEntity) {
        if (tEntity.eType != EntityType::Projectile)
            return;
        iLeft += tEntity.tBody.vVel.fX < -kBulletDef.fSpeed * 0.9f ? 1 : 0;
        iUp += tEntity.tBody.vVel.fY < -kBulletDef.fSpeed * 0.9f ? 1 : 0;
    });
    CHECK(iLeft == 1);
    CHECK(iUp == 1);
}

TEST_CASE("Projectile: 탄 1000발 생성·삭제 반복해도 World 풀이 버팀 (재사용)")
{
    Arena tArena;
    for (int iRound = 0; iRound < 100; ++iRound)
    {
        for (int i = 0; i < 10; ++i)
            tArena.tProjectiles.Spawn(tArena.tWorld, {300.0f, 300.0f}, {1.0f, 0.0f}, kBulletDef, Faction::Enemy);
        tArena.tWorld.ForEachAlive([&](Handle hEntity, const Entity&) { tArena.tWorld.Destroy(hEntity); });
        tArena.tWorld.FlushRemovals();
    }
    CHECK(tArena.tWorld.GetAliveCount() == 0);
}

TEST_CASE("Shooter: 감지 -> 가까우면 물러남 -> 사거리면 예고 뒤 플레이어 쪽으로 1발")
{
    World tWorld;
    const ShooterAI tAI;
    const Handle hPlayer = tWorld.Create(MakePlayerEntity({400.0f, 300.0f}));
    const Handle hShooter = tWorld.Create(MakeShooterEntity({500.0f, 340.0f}));  // 가로 약 100 = 너무 가까움

    tAI.Update(tWorld, hPlayer, kDt);
    CHECK(tWorld.Get(hShooter)->tAI.eState == EnemyState::Chase);
    tAI.Update(tWorld, hPlayer, kDt);
    CHECK(tWorld.Get(hShooter)->tMoveIntent.iMoveDir == 1);  // 플레이어(왼쪽) 반대로

    // 사거리 안 (가로 약 300) -> Windup -> 예고 시간(데이터 테이블) 뒤 발사
    tWorld.Get(hShooter)->tBody.vPos.fX = 700.0f;
    tAI.Update(tWorld, hPlayer, kDt);
    CHECK(tWorld.Get(hShooter)->tAI.eState == EnemyState::Windup);
    bool bFired = false;
    const int iMaxTicks = static_cast<int>(kShooterDef.fWindupTime / kDt) + 5;  // 수치가 바뀌어도 테스트는 그대로
    for (int i = 0; i < iMaxTicks && !bFired; ++i)
    {
        tAI.Update(tWorld, hPlayer, kDt);
        bFired = tWorld.Get(hShooter)->tAttackIntent.bFirePressed;
    }
    REQUIRE(bFired);
    CHECK(tWorld.Get(hShooter)->tAttackIntent.vAim.fX < 0.0f);  // 왼쪽(플레이어 쪽)
    CHECK(tWorld.Get(hShooter)->tAI.eState == EnemyState::Recover);
}

TEST_CASE("진영: Shooter·보스는 적 진영, 드랍·투사체는 맞는 몸이 아님")
{
    CHECK(IsEnemySide(EntityType::Shooter));
    CHECK(IsEnemySide(EntityType::Boss));
    CHECK_FALSE(IsEnemySide(EntityType::Projectile));
    CHECK_FALSE(IsHittable(EntityType::Drop));
    CHECK_FALSE(IsHittable(EntityType::Projectile));
    CHECK(GetFaction(EntityType::Shooter) == Faction::Enemy);
}
