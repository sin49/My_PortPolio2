#include "core/Game_C.h"

#include "raylib.h"

#include <utility>

Game::Game(Tilemap tStartRoom)
    : m_tRoom(std::move(tStartRoom))
{
}

void Game::Run()
{
    while (!WindowShouldClose())
    {
        m_tInput.Poll(m_tPlayerMoveIntent, m_tPlayerAttackIntent);

        // 고정 업데이트: 게임 로직은 항상 kStep(1/60s)만 받음
        const int iStepsThisFrame = m_tTimestep.Advance(GetFrameTime());
        for (int i = 0; i < iStepsThisFrame; ++i)
        {
            FixedUpdate(FixedTimestep::kStep);
            // 래치된 입력은 첫 틱이 소비
            m_tPlayerMoveIntent.bJumpPressed = false;
            m_tPlayerAttackIntent.bAttackPressed = false;
        }

        // 렌더: 매 프레임
        const DebugInfo tDebug = {.iStepsThisFrame = iStepsThisFrame, .llTotalTicks = m_llTotalTicks};
        m_tRender.Draw(m_tRoom, m_tPlayerBody, m_tPlayerMotorState, m_tPlayerAttack, tDebug);
    }
}

void Game::FixedUpdate(float fDt)
{
    m_tPlayerMotor.Update(m_tPlayerMoveIntent, m_tPlayerBody, m_tPlayerMotorState, kPlayerDef.fMoveSpeed, fDt);
    m_tPhysics.Update(m_tPlayerBody, m_tRoom, fDt);
    m_tCombat.UpdatePlayerAttack(m_tPlayerAttackIntent, m_tPlayerBody, m_tPlayerMotorState.iFacing, m_tPlayerAttack,
                                 fDt);
    ++m_llTotalTicks;
}
