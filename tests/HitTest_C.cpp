// 피격 판정 + SpatialQuery 테스트 (docs/design/hp-and-hit.md 5장)
#include "core/FixedTimestep_C.h"
#include "core/World_U.h"
#include "data/EnemyDef_C.h"
#include "data/PlayerDef_C.h"
#include "systems/Combat_C.h"
#include "systems/SpatialQuery_C.h"

#include <vector>

#include <doctest/doctest.h>

namespace
{
constexpr float kDt = FixedTimestep::kStep;
constexpr AttackIntent kNone = {};
constexpr AttackIntent kPress = {.bAttackPressed = true};

Entity MakeEntity(EntityType eType, Vec2 vPos, Vec2 vSize, float fHp)
{
    Entity tEntity;
    tEntity.eType = eType;
    tEntity.tBody = {.vPos = vPos, .vSize = vSize};
    tEntity.fHp = fHp;
    return tEntity;
}

// 플레이어(오른쪽 보는 중) 바로 앞에 적 하나. 히트박스 x 124~164 / 적 x 130~158
struct Arena
{
    World tWorld;
    SpatialQuery tSpatial{tWorld};
    Combat tCombat;
    std::vector<HitReport> vecReports;
    Handle hPlayer = tWorld.Create(MakeEntity(EntityType::Player, {100.0f, 200.0f}, kPlayerDef.vSize, 100.0f));
    Handle hEnemy = tWorld.Create(MakeEntity(EntityType::Enemy, {130.0f, 200.0f}, kChaserDef.vSize, 30.0f));

    // Game.FixedUpdate와 같은 순서 (이동·물리 제외)
    void Tick(const AttackIntent& tIntent)
    {
        Entity* pPlayer = tWorld.Get(hPlayer);
        tCombat.UpdateAttack(tIntent, pPlayer->tBody, pPlayer->tMotor.iFacing, kPlayerDef.tAttack, pPlayer->tAttack,
                             kDt);
        tSpatial.Rebuild();
        vecReports.clear();
        tCombat.ResolveHits(tWorld, tSpatial, vecReports, kDt);
        tWorld.FlushRemovals();
    }

    void FinishSwing()
    {
        for (int i = 0; i < 60; ++i)
            Tick(kNone);
    }

    float EnemyHp() { return tWorld.Get(hEnemy)->fHp; }
};
} // namespace

TEST_CASE("SpatialQuery: 겹치는 엔티티만, Flush된 것은 제외")
{
    World tWorld;
    const SpatialQuery tSpatial(tWorld);
    const Handle hNear = tWorld.Create(MakeEntity(EntityType::Enemy, {10.0f, 10.0f}, {20.0f, 20.0f}, 1.0f));
    tWorld.Create(MakeEntity(EntityType::Enemy, {500.0f, 10.0f}, {20.0f, 20.0f}, 1.0f));

    std::vector<Handle> vecOut;
    tSpatial.QueryNearby(Rect{0.0f, 0.0f, 50.0f, 50.0f}, vecOut);
    REQUIRE(vecOut.size() == 1);
    CHECK(vecOut[0] == hNear);

    tWorld.Destroy(hNear);
    tWorld.FlushRemovals();
    tSpatial.QueryNearby(Rect{0.0f, 0.0f, 50.0f, 50.0f}, vecOut);
    CHECK(vecOut.empty());
}

TEST_CASE("Hit: 한 번 휘두르면 1회만 맞음, 다음 휘두름엔 다시 맞음")
{
    Arena tArena;
    tArena.Tick(kPress);
    CHECK(tArena.EnemyHp() == doctest::Approx(30.0f - kPlayerDef.fAttackDamage));
    REQUIRE(tArena.vecReports.size() == 1);
    CHECK(tArena.vecReports[0].iDir == 1);

    tArena.Tick(kNone);  // 아직 Active지만 이미 맞은 대상
    tArena.Tick(kNone);
    CHECK(tArena.EnemyHp() == doctest::Approx(30.0f - kPlayerDef.fAttackDamage));
    CHECK(tArena.vecReports.empty());

    tArena.FinishSwing();
    tArena.tWorld.Get(tArena.hEnemy)->tBody.vPos = {130.0f, 200.0f};  // 넉백 속도는 있지만 이동(Physics)은 안 돌려서 제자리
    tArena.Tick(kPress);
    CHECK(tArena.EnemyHp() == doctest::Approx(30.0f - 2.0f * kPlayerDef.fAttackDamage));
}

TEST_CASE("Hit: 넉백 + 깜빡임")
{
    Arena tArena;
    tArena.Tick(kPress);
    const Entity* pEnemy = tArena.tWorld.Get(tArena.hEnemy);
    CHECK(pEnemy->tBody.vVel.fX > 0.0f);  // 플레이어가 오른쪽을 봄 -> 오른쪽으로 밀림
    CHECK(pEnemy->tBody.vVel.fY < 0.0f);  // 살짝 뜸
    CHECK(pEnemy->tHit.fFlashTimer > 0.0f);
}

TEST_CASE("Hit: HP 0이면 적은 삭제, 플레이어는 삭제 안 됨")
{
    Arena tArena;
    for (int i = 0; i < 3; ++i)  // 30 HP / 10 데미지
    {
        tArena.Tick(kPress);
        REQUIRE(tArena.vecReports.size() == 1);
        CHECK(tArena.vecReports[0].bKilled == (i == 2));  // 마지막 타격만 처치
        tArena.FinishSwing();
    }
    CHECK(tArena.tWorld.Get(tArena.hEnemy) == nullptr);

    Entity* pPlayer = tArena.tWorld.Get(tArena.hPlayer);
    REQUIRE(pPlayer != nullptr);
    pPlayer->fHp = 0.0f;  // 플레이어 사망 상태로 만들어도
    tArena.Tick(kNone);
    CHECK(tArena.tWorld.Get(tArena.hPlayer) != nullptr);  // 게임 오버는 RoomFlow 몫
}

TEST_CASE("Hit: 적끼리는 안 때림, 겹치지 않으면 안 맞음")
{
    World tWorld;
    const SpatialQuery tSpatial(tWorld);
    Combat tCombat;
    std::vector<HitReport> vecReports;

    // 적이 공격 중이고 바로 앞에 다른 적
    const Handle hAttacker = tWorld.Create(MakeEntity(EntityType::Enemy, {100.0f, 200.0f}, kChaserDef.vSize, 30.0f));
    const Handle hOther = tWorld.Create(MakeEntity(EntityType::Enemy, {130.0f, 200.0f}, kChaserDef.vSize, 30.0f));
    const Handle hFarPlayer = tWorld.Create(MakeEntity(EntityType::Player, {600.0f, 200.0f}, kPlayerDef.vSize, 100.0f));

    Entity* pAttacker = tWorld.Get(hAttacker);
    tCombat.UpdateAttack(kPress, pAttacker->tBody, 1, kChaserDef.tAttack, pAttacker->tAttack, kDt);
    tCombat.ResolveHits(tWorld, tSpatial, vecReports, kDt);

    CHECK(tWorld.Get(hOther)->fHp == doctest::Approx(30.0f));
    CHECK(tWorld.Get(hFarPlayer)->fHp == doctest::Approx(100.0f));
    CHECK(vecReports.empty());
}
