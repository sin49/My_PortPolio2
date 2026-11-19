#include "core/Game_C.h"

#include "data/EnemyDef_C.h"
#include "data/PlayerDef_C.h"

#include "raylib.h"

#include <algorithm>

namespace
{
constexpr float kPickupNoticeTime = 1.6f;

float GetMoveSpeed(const Entity& tEntity, const StatBlock& tPlayerStats)
{
    return tEntity.eType == EntityType::Player ? tPlayerStats.fMoveSpeed : kChaserDef.fMoveSpeed;
}

// 움직이는 몸(플레이어·적)만 Motor를 탐. 드랍은 중력(Physics)만
bool UsesMotor(const Entity& tEntity)
{
    return tEntity.eType == EntityType::Player || tEntity.eType == EntityType::Enemy;
}

// 플레이어 공격 = 기본 AttackDef의 히트박스를 공격 범위 배율만큼 키운 복사본 (원본 kPlayerDef는 그대로)
AttackDef MakePlayerAttackDef(const StatBlock& tPlayerStats)
{
    AttackDef tDef = kPlayerDef.tAttack;
    tDef.vSize = {tDef.vSize.fX * tPlayerStats.fAttackRange, tDef.vSize.fY * tPlayerStats.fAttackRange};
    return tDef;
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
            .tItems = m_tItems,
            .tPlayerStats = m_tPlayerStats,
            .tPickupNotice = m_tPickupNotice,
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
    const int iRoomBefore = m_tRoomFlow.GetRoomIndex();
    const RoomState eRoomStateBefore = m_tRoomFlow.GetRoomState();
    m_tRoomFlow.Update(m_tWorld, m_hPlayer, m_tMenuIntent, fDt);
    if (m_tRoomFlow.GetPhase() != GamePhase::Playing)
        return;

    // 새 게임(플레이어 핸들이 바뀜) -> 아이템 초기화
    if (!(m_hStatsPlayer == m_hPlayer))
    {
        m_tItems.Reset();
        m_tPlayerStats = kPlayerBaseStats;
        m_tPickupNotice = {};
        m_hStatsPlayer = m_hPlayer;
    }
    // 같은 방이 이번 틱에 Locked -> Cleared: 마지막 적이 죽은 자리에 보상 (RoomFlow는 아이템을 모름)
    if (iRoomBefore == m_tRoomFlow.GetRoomIndex() && eRoomStateBefore == RoomState::Locked &&
        m_tRoomFlow.GetRoomState() == RoomState::Cleared)
        m_tItems.SpawnClearReward(m_tWorld, m_vLastKillPos);

    // 줍기 -> 최종 스탯 재계산 (기본값 + modifier, 매 틱)
    m_vecPickups.clear();
    m_tItems.Update(m_tWorld, m_hPlayer, m_tPlayerStats.fMaxHp, fDt, m_vecPickups);
    for (const PickupReport& tPickup : m_vecPickups)
    {
        m_tPickupNotice = {.eItem = tPickup.eItem, .fTimer = kPickupNoticeTime};
        m_tParticles.SpawnHitSparks(tPickup.vPos, 1, true, 0.6f);
    }
    m_tPickupNotice.fTimer = std::max(0.0f, m_tPickupNotice.fTimer - fDt);
    UpdatePlayerStats();

    // 적 의도 -> 속도 -> 이동·충돌 -> 공간 갱신 -> 공격 단계 -> 판정 -> 연출 -> 삭제 (GDD 7장 순서)
    // (플레이어 의도는 렌더 프레임에서 Input이 이미 기록)
    m_tEnemyAI.Update(m_tWorld, m_hPlayer, fDt);
    m_tWorld.ForEachAlive([&](Handle, Entity& tEntity) {
        if (UsesMotor(tEntity))
            m_tPlayerMotor.Update(tEntity.tMoveIntent, tEntity.tBody, tEntity.tMotor,
                                  GetMoveSpeed(tEntity, m_tPlayerStats), fDt);
    });
    m_tWorld.ForEachAlive([&](Handle, Entity& tEntity) {
        m_tPhysics.Update(tEntity.tBody, m_tRoom, fDt);
        if (tEntity.tMotor.tDash.IsDashing())  // 연출: 대시 잔상 (판정 무관)
            m_tParticles.SpawnGhost(GetBounds(tEntity.tBody));
    });
    m_tSpatial.Rebuild();
    const AttackDef tPlayerAttack = MakePlayerAttackDef(m_tPlayerStats);
    m_tWorld.ForEachAlive([&](Handle, Entity& tEntity) {
        if (!UsesMotor(tEntity))
            return;
        const AttackDef& tDef = tEntity.eType == EntityType::Player ? tPlayerAttack : kChaserDef.tAttack;
        m_tCombat.UpdateAttack(tEntity.tAttackIntent, tEntity.tBody, tEntity.tMotor.iFacing, tDef, tEntity.tAttack,
                               fDt);
    });

    m_vecHitReports.clear();
    m_tCombat.ResolveHits(m_tWorld, m_tSpatial, m_vecHitReports, fDt, m_tPlayerStats.fAttackDamage);
    for (const HitReport& tReport : m_vecHitReports)
        m_tParticles.SpawnHitSparks(tReport.vPos, tReport.iDir, tReport.bKilled, tReport.fScale);
    HandleKillDrops();
    m_tParticles.Update(fDt);

    m_tWorld.FlushRemovals();  // 삭제는 틱 끝에서 일괄 (순회 중 삭제 금지)
    ++m_llTotalTicks;
}

void Game::UpdatePlayerStats()
{
    m_tItems.BuildModifiers(m_vecStatMods);
    const StatBlock tNew = Stats::Calculate(kPlayerBaseStats, m_vecStatMods);  // 원본은 const -> 안 바뀜
    if (Entity* pPlayer = m_tWorld.Get(m_hPlayer))
    {
        // 최대 HP가 늘면 늘어난 만큼 현재 HP도 + (items.md 결정 6)
        if (tNew.fMaxHp > m_tPlayerStats.fMaxHp && pPlayer->fHp > 0.0f)
            pPlayer->fHp += tNew.fMaxHp - m_tPlayerStats.fMaxHp;
        pPlayer->fHp = std::min(pPlayer->fHp, tNew.fMaxHp);
    }
    m_tPlayerStats = tNew;
}

void Game::HandleKillDrops()
{
    const Entity* pPlayer = m_tWorld.Get(m_hPlayer);
    if (pPlayer == nullptr)
        return;
    const float fHpRatio = pPlayer->fHp / m_tPlayerStats.fMaxHp;
    for (const HitReport& tReport : m_vecHitReports)
    {
        if (!tReport.bKilled || tReport.bTargetIsPlayer)
            continue;
        m_vLastKillPos = tReport.vPos;
        m_tItems.TrySpawnPotion(m_tWorld, tReport.vPos, fHpRatio);  // HP가 낮을수록 잘 나옴 (가득이면 0%)
    }
}
