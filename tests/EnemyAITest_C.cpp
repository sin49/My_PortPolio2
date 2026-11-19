// EnemyAI 스펙 테스트 (AI 작성). 사용자가 구현한 EnemyAI가 이 테스트를 통과하면 완료.
// 기준: docs/design/chaser.md. 테스트가 설계와 다르다고 생각되면 고치지 말고 먼저 논의.
#include "core/FixedTimestep_C.h"
#include "core/World_U.h"
#include "data/EnemyDef_C.h"
#include "data/PlayerDef_C.h"
#include "systems/EnemyAI_U.h"

#include <doctest/doctest.h>

namespace
{
constexpr float kDt = FixedTimestep::kStep;
constexpr float kFloorY = 600.0f;  // 같은 층: 둘 다 발이 y 640 근처

Entity MakePlayer(float fX, float fY = kFloorY)
{
    Entity tEntity;
    tEntity.eType = EntityType::Player;
    tEntity.tBody = {.vPos = {fX, fY}, .vSize = kPlayerDef.vSize};
    tEntity.fHp = 100.0f;
    return tEntity;
}

Entity MakeChaser(float fX)
{
    Entity tEntity;
    tEntity.eType = EntityType::Enemy;
    tEntity.tBody = {.vPos = {fX, kFloorY + 4.0f}, .vSize = kChaserDef.vSize};  // 키 차이만큼 내려서 발 맞춤
    tEntity.fHp = kChaserDef.fMaxHp;
    return tEntity;
}

struct Scene
{
    World tWorld;
    EnemyAI tAI;
    Handle hPlayer;
    Handle hEnemy;

    Scene(float fPlayerX, float fEnemyX, float fPlayerY = kFloorY)
    {
        hPlayer = tWorld.Create(MakePlayer(fPlayerX, fPlayerY));
        hEnemy = tWorld.Create(MakeChaser(fEnemyX));
    }

    void Tick() { tAI.Update(tWorld, hPlayer, kDt); }
    Entity& Enemy() { return *tWorld.Get(hEnemy); }
    EnemyState State() { return Enemy().tAI.eState; }

    // 상태가 바뀔 때까지 진행. 걸린 틱 수 (최대 iMaxTicks)
    int RunWhile(EnemyState eState, int iMaxTicks = 120)
    {
        int iTicks = 0;
        while (State() == eState && iTicks < iMaxTicks)
        {
            Tick();
            ++iTicks;
        }
        return iTicks;
    }
};
} // namespace

TEST_CASE("EnemyAI: 멀면 Idle 유지, 의도 없음")
{
    Scene tScene(100.0f, 100.0f + kChaserDef.fDetectRange + 100.0f);
    for (int i = 0; i < 10; ++i)
        tScene.Tick();
    CHECK(tScene.State() == EnemyState::Idle);
    CHECK(tScene.Enemy().tMoveIntent.iMoveDir == 0);
    CHECK_FALSE(tScene.Enemy().tAttackIntent.bAttackPressed);
}

TEST_CASE("EnemyAI: 감지 거리 안 -> Chase, 플레이어 쪽으로 이동 의도")
{
    Scene tScene(100.0f, 400.0f);  // 플레이어가 왼쪽
    tScene.Tick();
    CHECK(tScene.State() == EnemyState::Chase);
    tScene.Tick();
    CHECK(tScene.Enemy().tMoveIntent.iMoveDir == -1);

    Scene tRight(700.0f, 400.0f);  // 플레이어가 오른쪽
    tRight.Tick();
    tRight.Tick();
    CHECK(tRight.Enemy().tMoveIntent.iMoveDir == 1);
}

TEST_CASE("EnemyAI: 붙으면 Windup -> 공격 1틱 -> Recover -> Chase")
{
    // 플레이어 몸 100~124, 적 몸 140~168 -> 간격 16 <= fAttackRangeX
    Scene tScene(100.0f, 140.0f);
    tScene.Tick();  // Idle -> Chase
    tScene.Tick();  // Chase -> Windup
    REQUIRE(tScene.State() == EnemyState::Windup);

    // 예고 중: 움직이지 않음, 공격도 아직
    tScene.Tick();
    CHECK(tScene.Enemy().tMoveIntent.iMoveDir == 0);
    CHECK_FALSE(tScene.Enemy().tAttackIntent.bAttackPressed);

    const int iWindupTicks = tScene.RunWhile(EnemyState::Windup) + 1;  // 0.4초 ≈ 24틱
    CHECK(iWindupTicks >= 22);
    CHECK(iWindupTicks <= 26);

    // Attack 상태에서 딱 1틱 누름 -> 바로 Recover
    int iPressedTicks = 0;
    for (int i = 0; i < 3; ++i)
    {
        tScene.Tick();
        if (tScene.Enemy().tAttackIntent.bAttackPressed)
            ++iPressedTicks;
    }
    CHECK(iPressedTicks == 1);
    CHECK(tScene.State() == EnemyState::Recover);

    const int iRecoverTicks = tScene.RunWhile(EnemyState::Recover);  // 0.6초 ≈ 36틱
    CHECK(iRecoverTicks >= 32);
    CHECK(iRecoverTicks <= 38);
    CHECK(tScene.State() == EnemyState::Chase);
}

TEST_CASE("EnemyAI: 다른 층이면 붙어도 공격 안 함")
{
    Scene tScene(150.0f, 140.0f, kFloorY - 200.0f);  // 가로로는 겹치지만 플레이어가 위층
    for (int i = 0; i < 30; ++i)
        tScene.Tick();
    CHECK(tScene.State() == EnemyState::Chase);
}

TEST_CASE("EnemyAI: 멀어지면 Chase -> Idle")
{
    Scene tScene(100.0f, 400.0f);
    tScene.Tick();
    REQUIRE(tScene.State() == EnemyState::Chase);

    tScene.tWorld.Get(tScene.hPlayer)->tBody.vPos.fX = 400.0f + kChaserDef.fLoseRange + 100.0f;
    tScene.Tick();
    CHECK(tScene.State() == EnemyState::Idle);
}

TEST_CASE("EnemyAI: 의도만 씀 - 위치·속도·HP는 그대로")
{
    Scene tScene(100.0f, 300.0f);
    const Body tBefore = tScene.Enemy().tBody;
    for (int i = 0; i < 60; ++i)
        tScene.Tick();
    const Entity& tEnemy = tScene.Enemy();
    CHECK(tEnemy.tBody.vPos.fX == tBefore.vPos.fX);
    CHECK(tEnemy.tBody.vPos.fY == tBefore.vPos.fY);
    CHECK(tEnemy.tBody.vVel.fX == tBefore.vVel.fX);
    CHECK(tEnemy.fHp == kChaserDef.fMaxHp);
    CHECK(tScene.tWorld.Get(tScene.hPlayer)->fHp == 100.0f);
}

TEST_CASE("EnemyAI: HP 0인 적(삭제 예약 중)은 건너뜀")
{
    Scene tScene(100.0f, 300.0f);
    tScene.Enemy().fHp = 0.0f;
    tScene.Tick();
    CHECK(tScene.State() == EnemyState::Idle);
}

TEST_CASE("EnemyAI: 허수아비(bPassive)는 가까워도 아무것도 안 함")
{
    Scene tScene(100.0f, 140.0f);  // 바로 붙어 있음
    tScene.Enemy().tAI.bPassive = true;
    for (int i = 0; i < 60; ++i)
        tScene.Tick();
    CHECK(tScene.State() == EnemyState::Idle);
    CHECK(tScene.Enemy().tMoveIntent.iMoveDir == 0);
    CHECK_FALSE(tScene.Enemy().tAttackIntent.bAttackPressed);
}

TEST_CASE("EnemyAI: 플레이어 핸들이 무효여도 안전 (Idle 유지)")
{
    World tWorld;
    EnemyAI tAI;
    const Handle hEnemy = tWorld.Create(MakeChaser(300.0f));
    tAI.Update(tWorld, Handle{}, kDt);
    CHECK(tWorld.Get(hEnemy)->tAI.eState == EnemyState::Idle);
}
