#include "core/Game_C.h"

#include "data/EnemyDef_C.h"
#include "data/PlayerDef_C.h"

#include "raylib.h"

namespace
{
float GetMoveSpeed(const Entity& tEntity)  // TODO(Stats): 최종 이동 속도
{
    return tEntity.eType == EntityType::Player ? kPlayerDef.fMoveSpeed : kChaserDef.fMoveSpeed;
}

Vec2 GetRoomSizePx(const Tilemap& tRoom)
{
    return Vec2{static_cast<float>(tRoom.GetCols()) * kTileSize, static_cast<float>(tRoom.GetRows()) * kTileSize};
}
} // namespace

Game::Game()
{
    m_vecHitReports.reserve(kMaxHitsPerSwing);
    // 타이틀 화면 뒤 배경용으로 첫 방 + 플레이어를 미리 깔아 둠. Enter 누르면 RoomFlow가 새로 시작
    m_tRoomFlow.PrepareTitle(m_tWorld, m_hPlayer);
}

void Game::Run()
{
    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_F1))
            m_bShowDebug = !m_bShowDebug;

        m_tInput.PollMenu(m_tMenuIntent);
        if (Entity* pPlayer = m_tWorld.Get(m_hPlayer))
            m_tInput.Poll(pPlayer->tMoveIntent, pPlayer->tAttackIntent);

        // 고정 업데이트: 게임 로직은 항상 kStep(1/60s)만 받음
        const int iStepsThisFrame = m_tTimestep.Advance(GetFrameTime());
        for (int i = 0; i < iStepsThisFrame; ++i)
        {
            FixedUpdate(FixedTimestep::kStep);
            // 래치된 입력은 첫 틱이 소비 (Flush·방 전환 뒤라 다시 꺼냄)
            m_tMenuIntent.bConfirmPressed = false;
            if (Entity* pPlayer = m_tWorld.Get(m_hPlayer))
            {
                pPlayer->tMoveIntent.bJumpPressed = false;
                pPlayer->tMoveIntent.bDashPressed = false;
                pPlayer->tAttackIntent.bAttackPressed = false;
            }
        }

        UpdateCamera(GetFrameTime());

        // 렌더: 매 프레임
        const FrameView tView = {
            .tRoom = m_tRoom,
            .tWorld = m_tWorld,
            .hPlayer = m_hPlayer,
            .tParticles = m_tParticles,
            .tCamera = m_tCamera,
            .tRoomFlow = m_tRoomFlow,
            .tDebug = {.iStepsThisFrame = iStepsThisFrame, .llTotalTicks = m_llTotalTicks, .bShow = m_bShowDebug},
        };
        m_tRender.Draw(tView);
    }
}

void Game::UpdateCamera(float fFrameDt)
{
    const Entity* pPlayer = m_tWorld.Get(m_hPlayer);
    if (pPlayer == nullptr)
        return;
    const Body& tBody = pPlayer->tBody;
    const Vec2 vFocus = {tBody.vPos.fX + tBody.vSize.fX * 0.5f, tBody.vPos.fY + tBody.vSize.fY * 0.5f};
    const Vec2 vRoom = GetRoomSizePx(m_tRoom);

    const bool bRoomChanged = m_iCameraRoomIndex != m_tRoomFlow.GetRoomIndex() || !(m_hCameraPlayer == m_hPlayer);
    if (bRoomChanged)
    {
        m_tCamera.SnapTo(vFocus, kViewSize, vRoom);
        m_iCameraRoomIndex = m_tRoomFlow.GetRoomIndex();
        m_hCameraPlayer = m_hPlayer;
        return;
    }
    m_tCamera.Update(vFocus, kViewSize, vRoom, fFrameDt);
}

void Game::FixedUpdate(float fDt)
{
    // 방 전환·승패 먼저. 플레이 중이 아니면(타이틀·게임 오버·승리) 시뮬레이션 정지
    m_tRoomFlow.Update(m_tWorld, m_hPlayer, m_tMenuIntent, fDt);
    if (m_tRoomFlow.GetPhase() != GamePhase::Playing)
        return;

    // 의도 -> 속도 -> 이동·충돌 -> 공간 갱신 -> 공격 단계 -> 판정 -> 연출 -> 삭제 (GDD 7장 순서)
    m_tWorld.ForEachAlive([&](Handle, Entity& tEntity) {
        m_tPlayerMotor.Update(tEntity.tMoveIntent, tEntity.tBody, tEntity.tMotor, GetMoveSpeed(tEntity), fDt);
    });
    m_tWorld.ForEachAlive([&](Handle, Entity& tEntity) {
        m_tPhysics.Update(tEntity.tBody, m_tRoom, fDt);
        if (tEntity.tMotor.tDash.IsDashing())  // 연출: 대시 잔상 (판정 무관)
            m_tParticles.SpawnGhost(GetBounds(tEntity.tBody));
    });
    m_tSpatial.Rebuild();
    m_tWorld.ForEachAlive([&](Handle, Entity& tEntity) {
        const AttackDef& tDef = tEntity.eType == EntityType::Player ? kPlayerDef.tAttack : kChaserDef.tAttack;
        m_tCombat.UpdateAttack(tEntity.tAttackIntent, tEntity.tBody, tEntity.tMotor.iFacing, tDef, tEntity.tAttack,
                               fDt);
    });

    m_vecHitReports.clear();
    m_tCombat.ResolveHits(m_tWorld, m_tSpatial, m_vecHitReports, fDt);
    for (const HitReport& tReport : m_vecHitReports)
        m_tParticles.SpawnHitSparks(tReport.vPos, tReport.iDir, tReport.bKilled, tReport.fScale);
    m_tParticles.Update(fDt);

    m_tWorld.FlushRemovals();  // 삭제는 틱 끝에서 일괄 (순회 중 삭제 금지)
    ++m_llTotalTicks;
}
