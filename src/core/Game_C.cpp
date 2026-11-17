#include "core/Game_C.h"

#include "data/EnemyDef_C.h"
#include "data/PlayerDef_C.h"
#include "data/TestRoom_C.h"

#include "raylib.h"

#include <utility>

namespace
{
Entity MakePlayer()
{
    Entity tPlayer;
    tPlayer.eType = EntityType::Player;
    tPlayer.tBody = {.vPos = kTestRoomPlayerSpawn, .vSize = kPlayerDef.vSize};
    tPlayer.fHp = kPlayerDef.fMaxHp;
    return tPlayer;
}

Entity MakeChaser(Vec2 vPos)
{
    Entity tEnemy;
    tEnemy.eType = EntityType::Enemy;
    tEnemy.tBody = {.vPos = vPos, .vSize = kChaserDef.vSize};
    tEnemy.fHp = kChaserDef.fMaxHp;
    tEnemy.tMotor.iFacing = -1;
    return tEnemy;
}

float GetMoveSpeed(const Entity& tEntity)  // TODO(Stats): 최종 이동 속도
{
    return tEntity.eType == EntityType::Player ? kPlayerDef.fMoveSpeed : kChaserDef.fMoveSpeed;
}
} // namespace

Game::Game(Tilemap tStartRoom)
    : m_tRoom(std::move(tStartRoom))
{
    m_vecHitReports.reserve(kMaxHitsPerSwing);
    m_hPlayer = m_tWorld.Create(MakePlayer());
    for (const Vec2& vSpawn : kTestRoomEnemySpawns)  // 허수아비 (AI 없음). TODO(RoomFlow): 방 데이터에서
        m_tWorld.Create(MakeChaser(vSpawn));
}

void Game::Run()
{
    while (!WindowShouldClose())
    {
        if (Entity* pPlayer = m_tWorld.Get(m_hPlayer))
            m_tInput.Poll(pPlayer->tMoveIntent, pPlayer->tAttackIntent);

        // 고정 업데이트: 게임 로직은 항상 kStep(1/60s)만 받음
        const int iStepsThisFrame = m_tTimestep.Advance(GetFrameTime());
        for (int i = 0; i < iStepsThisFrame; ++i)
        {
            FixedUpdate(FixedTimestep::kStep);
            // 래치된 입력은 첫 틱이 소비 (Flush 뒤라 다시 꺼냄)
            if (Entity* pPlayer = m_tWorld.Get(m_hPlayer))
            {
                pPlayer->tMoveIntent.bJumpPressed = false;
                pPlayer->tAttackIntent.bAttackPressed = false;
            }
        }

        // 렌더: 매 프레임
        const DebugInfo tDebug = {.iStepsThisFrame = iStepsThisFrame, .llTotalTicks = m_llTotalTicks};
        m_tRender.Draw(m_tRoom, m_tWorld, m_hPlayer, m_tParticles, tDebug);
    }
}

void Game::FixedUpdate(float fDt)
{
    // 의도 -> 속도 -> 이동·충돌 -> 공간 갱신 -> 공격 단계 -> 판정 -> 연출 -> 삭제 (GDD 7장 순서)
    m_tWorld.ForEachAlive([&](Handle, Entity& tEntity) {
        m_tPlayerMotor.Update(tEntity.tMoveIntent, tEntity.tBody, tEntity.tMotor, GetMoveSpeed(tEntity), fDt);
    });
    m_tWorld.ForEachAlive([&](Handle, Entity& tEntity) { m_tPhysics.Update(tEntity.tBody, m_tRoom, fDt); });
    m_tSpatial.Rebuild();
    m_tWorld.ForEachAlive([&](Handle, Entity& tEntity) {
        m_tCombat.UpdatePlayerAttack(tEntity.tAttackIntent, tEntity.tBody, tEntity.tMotor.iFacing, tEntity.tAttack,
                                     fDt);
    });

    m_vecHitReports.clear();
    m_tCombat.ResolveHits(m_tWorld, m_tSpatial, m_vecHitReports, fDt);
    for (const HitReport& tReport : m_vecHitReports)
        m_tParticles.SpawnHitSparks(tReport.vPos, tReport.iDir, tReport.bKilled);
    m_tParticles.Update(fDt);

    m_tWorld.FlushRemovals();  // 삭제는 틱 끝에서 일괄 (순회 중 삭제 금지)
    ++m_llTotalTicks;
}
