// 보스 FSM 테스트 (boss-projectile.md 4장)
#include "core/FixedTimestep_C.h"
#include "core/World_U.h"
#include "data/EnemyDef_C.h"
#include "data/EntityFactory_C.h"
#include "data/ProjectileDef_C.h"
#include "systems/BossAI_C.h"
#include "systems/Projectile_C.h"

#include <doctest/doctest.h>

#include <ostream>
#include <vector>

namespace
{
constexpr float kDt = FixedTimestep::kStep;

struct Arena
{
    World tWorld;
    BossAI tBossAI;
    Projectiles tProjectiles;
    Handle hPlayer = tWorld.Create(MakePlayerEntity({100.0f, 600.0f}));
    Handle hBoss = tWorld.Create(MakeBossEntity({600.0f, 672.0f}));

    Entity& Boss() { return *tWorld.Get(hBoss); }

    // AI 1틱 + 발사 의도 -> 탄 생성 (Game 순서). 반환: 이번 틱에 생긴 탄 수
    int Tick()
    {
        tBossAI.Update(tWorld, hPlayer, kDt);
        const int iBefore = tWorld.GetAliveCount();
        tProjectiles.FireFromIntents(tWorld, kBulletDef);
        return tWorld.GetAliveCount() - iBefore;
    }

    // 상태가 eTarget이 될 때까지 (최대 iMaxTicks). 그동안 생긴 탄 수 합
    int TickUntil(BossState eTarget, int iMaxTicks = 600)
    {
        int iBullets = 0;
        for (int i = 0; i < iMaxTicks && Boss().tBoss.eState != eTarget; ++i)
            iBullets += Tick();
        return iBullets;
    }
};
} // namespace

TEST_CASE("BossAI: 패턴 순서 = 조준 3발 -> 쉼 -> 원형 8발 -> 쉼 -> 다시 조준")
{
    Arena tArena;
    CHECK(tArena.Boss().tBoss.eState == BossState::Idle);
    tArena.Tick();
    REQUIRE(tArena.Boss().tBoss.eState == BossState::AimedWindup);  // 가로 500px < 감지 900px

    CHECK(tArena.TickUntil(BossState::Recover) == kBossDef.iAimedShots);
    CHECK(tArena.Boss().tBoss.bNextIsRing);
    CHECK(tArena.TickUntil(BossState::RingWindup) == 0);  // 쉬는 동안은 안 쏨
    CHECK(tArena.TickUntil(BossState::Recover) == kBossDef.iRingShots);
    CHECK(tArena.TickUntil(BossState::AimedWindup) == 0);
}

TEST_CASE("BossAI: 조준 3발의 가운데 탄은 플레이어 쪽 / 예고 시간은 데이터 테이블대로")
{
    Arena tArena;
    tArena.Tick();
    int iWindupTicks = 0;
    while (tArena.Boss().tBoss.eState == BossState::AimedWindup && iWindupTicks < 600)
    {
        tArena.Tick();
        ++iWindupTicks;
    }
    CHECK(static_cast<float>(iWindupTicks) * kDt == doctest::Approx(kBossDef.fAimedWindupTime).epsilon(0.05));
    REQUIRE(tArena.Boss().tBoss.eState == BossState::AimedFire);
    CHECK(tArena.Tick() == kBossDef.iAimedShots);  // 발사는 AimedFire 상태의 1틱

    // 방금 생긴 탄 3개: 모두 왼쪽(플레이어 쪽)으로
    int iLeftward = 0;
    tArena.tWorld.ForEachAlive([&](Handle, const Entity& tEntity) {
        if (tEntity.eType == EntityType::Projectile && tEntity.tBody.vVel.fX < 0.0f)
            ++iLeftward;
    });
    CHECK(iLeftward == kBossDef.iAimedShots);
}

TEST_CASE("BossAI: HP 절반 이하 = 2페이즈 -> 쉬는 시간이 짧아짐")
{
    Arena tArena;
    tArena.Tick();
    tArena.TickUntil(BossState::Recover);
    CHECK(tArena.Boss().tBoss.fTimer == doctest::Approx(kBossDef.fRecoverTime).epsilon(0.05));

    tArena.Boss().fHp = kBossDef.fMaxHp * kBossDef.fPhase2HpRatio;  // 딱 절반
    tArena.TickUntil(BossState::RingWindup);
    tArena.TickUntil(BossState::Recover);
    CHECK(tArena.Boss().tBoss.fTimer == doctest::Approx(kBossDef.fRecoverTimePhase2).epsilon(0.05));
}

TEST_CASE("BossAI: 의도만 씀 (이동은 쉬는 동안만 플레이어 쪽, 위치·HP는 안 바꿈)")
{
    Arena tArena;
    const Vec2 vStart = tArena.Boss().tBody.vPos;
    tArena.Tick();
    CHECK(tArena.Boss().tMoveIntent.iMoveDir == 0);  // 예고 중엔 멈춤
    tArena.TickUntil(BossState::Recover);
    tArena.Tick();
    CHECK(tArena.Boss().tMoveIntent.iMoveDir == -1);  // 플레이어는 왼쪽
    CHECK(tArena.Boss().tBody.vPos.fX == doctest::Approx(vStart.fX));  // Motor·Physics가 안 돌았으니 그대로
    CHECK(tArena.Boss().fHp == doctest::Approx(kBossDef.fMaxHp));
}

TEST_CASE("BossAI: 플레이어가 멀면 Idle, 보스 방 데이터는 Boss 타입으로 생성")
{
    World tWorld;
    BossAI tBossAI;
    const Handle hPlayer = tWorld.Create(MakePlayerEntity({0.0f, 600.0f}));
    const Handle hBoss = tWorld.Create(MakeEnemyEntity({.eKind = SpawnKind::Boss}, {1500.0f, 672.0f}));
    REQUIRE(tWorld.Get(hBoss)->eType == EntityType::Boss);
    CHECK(tWorld.Get(hBoss)->fHp == doctest::Approx(kBossDef.fMaxHp));
    tBossAI.Update(tWorld, hPlayer, kDt);
    CHECK(tWorld.Get(hBoss)->tBoss.eState == BossState::Idle);  // 가로 1500 > 900
}
