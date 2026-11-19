// 피격 무적 + 발판 끝 멈춤 (11/19 플레이 피드백)
#include "core/FixedTimestep_C.h"
#include "core/Tilemap_C.h"
#include "core/World_U.h"
#include "data/EnemyDef_C.h"
#include "data/EntityFactory_C.h"
#include "data/PlayerDef_C.h"
#include "data/ProjectileDef_C.h"
#include "systems/Combat_C.h"
#include "systems/LedgeGuard_C.h"
#include "systems/Projectile_C.h"
#include "systems/SpatialQuery_C.h"

#include <doctest/doctest.h>

#include <array>
#include <vector>

namespace
{
constexpr float kDt = FixedTimestep::kStep;

// 플레이어 몸에 겹친 히트박스로 지금 공격 중인 Chaser
Handle AddAttackingChaser(World& tWorld, Handle hPlayer)
{
    Entity tChaser = MakeEnemyEntity({.eKind = SpawnKind::Chaser}, {340.0f, 300.0f});
    tChaser.tAttack.ePhase = AttackPhase::Active;
    tChaser.tAttack.rcHitbox = GetBounds(tWorld.Get(hPlayer)->tBody);
    return tWorld.Create(tChaser);
}

void Resolve(World& tWorld, SpatialQuery& tSpatial, Combat& tCombat, float fDt = kDt)
{
    std::vector<HitReport> vecReports;
    tSpatial.Rebuild();
    tCombat.ResolveHits(tWorld, tSpatial, vecReports, fDt);
}
} // namespace

TEST_CASE("피격 무적: 겹친 적 3마리가 같은 틱에 때려도 1대만, 무적이 끝나면 다시 맞음")
{
    World tWorld;
    SpatialQuery tSpatial{tWorld};
    Combat tCombat;
    const Handle hPlayer = tWorld.Create(MakePlayerEntity({320.0f, 260.0f}));
    for (int i = 0; i < 3; ++i)
        AddAttackingChaser(tWorld, hPlayer);

    Resolve(tWorld, tSpatial, tCombat);
    const float fAfterFirst = tWorld.Get(hPlayer)->fHp;
    CHECK(fAfterFirst == doctest::Approx(kPlayerDef.fMaxHp - kChaserDef.fAttackDamage));
    CHECK(tWorld.Get(hPlayer)->tHit.fInvulnTimer > 0.0f);

    // 무적 중엔 새 공격(새 적)도 안 맞음
    AddAttackingChaser(tWorld, hPlayer);
    Resolve(tWorld, tSpatial, tCombat);
    CHECK(tWorld.Get(hPlayer)->fHp == doctest::Approx(fAfterFirst));

    // 무적 시간이 지나면 새 공격에 맞음
    Resolve(tWorld, tSpatial, tCombat, kPlayerDef.fHurtInvulnTime + 0.01f);  // 타이머만 줄이는 큰 틱
    AddAttackingChaser(tWorld, hPlayer);
    Resolve(tWorld, tSpatial, tCombat);
    CHECK(tWorld.Get(hPlayer)->fHp < fAfterFirst);
}

TEST_CASE("피격 무적: 적은 무적이 안 생김 (플레이어 공격은 그대로 연속으로 들어감)")
{
    World tWorld;
    SpatialQuery tSpatial{tWorld};
    Combat tCombat;
    const Handle hEnemy = tWorld.Create(MakeEnemyEntity({.eKind = SpawnKind::Chaser, .eSize = EnemySize::Large}, {300.0f, 300.0f}));
    Entity tPlayer = MakePlayerEntity({260.0f, 260.0f});
    tPlayer.tAttack.ePhase = AttackPhase::Active;
    tPlayer.tAttack.rcHitbox = GetBounds(tWorld.Get(hEnemy)->tBody);
    tWorld.Create(tPlayer);
    Resolve(tWorld, tSpatial, tCombat);
    CHECK(tWorld.Get(hEnemy)->tHit.fInvulnTimer == doctest::Approx(0.0f));
}

TEST_CASE("발판 끝: 땅 위 적이 바깥으로 가려 하면 이동 의도 0, 안쪽은 그대로, 플레이어·공중은 안 건드림")
{
    // 바닥 없는 넓은 공중 + 발판 하나 (x 10~14칸, y 10칸)
    static constexpr std::array<TileRect, 1> kSolids = {{{10, 10, 5, 1}}};
    const Tilemap tRoom = Tilemap::FromSolids(40, 22, kSolids);
    World tWorld;
    const LedgeGuard tGuard;

    // 발판 오른쪽 끝 근처 (발판 끝 = 15칸 = 480px)
    Entity tEnemy = MakeEnemyEntity({.eKind = SpawnKind::Chaser}, {470.0f, 320.0f});
    tEnemy.tBody.bGrounded = true;
    tEnemy.tMoveIntent.iMoveDir = 1;
    const Handle hOut = tWorld.Create(tEnemy);
    tEnemy.tMoveIntent.iMoveDir = -1;
    const Handle hIn = tWorld.Create(tEnemy);
    tEnemy.tMoveIntent.iMoveDir = 1;
    tEnemy.tBody.bGrounded = false;
    const Handle hAir = tWorld.Create(tEnemy);
    Entity tPlayer = MakePlayerEntity({460.0f, 272.0f});
    tPlayer.tBody.bGrounded = true;
    tPlayer.tMoveIntent.iMoveDir = 1;
    const Handle hPlayer = tWorld.Create(tPlayer);
    // 발판 가운데 (끝에서 멂)
    Entity tMiddle = MakeEnemyEntity({.eKind = SpawnKind::Chaser}, {384.0f, 320.0f});
    tMiddle.tBody.bGrounded = true;
    tMiddle.tMoveIntent.iMoveDir = 1;
    const Handle hMiddle = tWorld.Create(tMiddle);

    tGuard.Apply(tWorld, tRoom);
    CHECK(tWorld.Get(hOut)->tMoveIntent.iMoveDir == 0);
    CHECK(tWorld.Get(hIn)->tMoveIntent.iMoveDir == -1);
    CHECK(tWorld.Get(hAir)->tMoveIntent.iMoveDir == 1);
    CHECK(tWorld.Get(hPlayer)->tMoveIntent.iMoveDir == 1);
    CHECK(tWorld.Get(hMiddle)->tMoveIntent.iMoveDir == 1);
}
