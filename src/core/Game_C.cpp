#include "core/Game_C.h"

#include "core/EntitySide_C.h"

#include "data/EnemyDef_C.h"
#include "data/PlayerDef_C.h"
#include "data/ProjectileDef_C.h"

#include "raylib.h"

#include <algorithm>
#include <filesystem>

namespace
{
constexpr float kPickupNoticeTime = 1.6f;
constexpr float kPlayerHurtShake = 0.55f;  // 플레이어 피격 때 화면 흔들림 트라우마 (0~1)

float GetMoveSpeed(const Entity& tEntity, const StatBlock& tPlayerStats)
{
    if (tEntity.eType == EntityType::Player)
        return tPlayerStats.fMoveSpeed;
    if (tEntity.eType == EntityType::Boss)
        return kBossDef.fMoveSpeed;
    return tEntity.eType == EntityType::Shooter ? kShooterDef.fMoveSpeed : kChaserDef.fMoveSpeed;
}

// 움직이는 몸(플레이어·적)만 Motor를 탐. 드랍은 중력(Physics)만
bool UsesMotor(const Entity& tEntity)
{
    return IsHittable(tEntity.eType);  // 플레이어·Chaser·Shooter·보스
}

// 플레이어 공격 = 기본 AttackDef의 히트박스를 공격 범위 배율만큼 키운 복사본 (원본 kPlayerDef는 그대로)
AttackDef MakePlayerAttackDef(const StatBlock& tPlayerStats)
{
    AttackDef tDef = kPlayerDef.tAttack;
    tDef.vSize = {tDef.vSize.fX * tPlayerStats.fAttackRange, tDef.vSize.fY * tPlayerStats.fAttackRange};
    return tDef;
}
} // namespace

Game::Game(const GameOptions& tOptions)
{
    m_vecHitReports.reserve(kMaxHitsPerSwing);
    m_tSpatial.SetMode(tOptions.eSpatialMode);
    m_tSpatial.SetCellSize(tOptions.fCellSize);
    // 타이틀 화면 뒤 배경용으로 첫 방 + 플레이어를 미리 깔아 둠. Enter 누르면 RoomFlow가 새로 시작
    m_tRoomFlow.PrepareTitle(m_tWorld, m_hPlayer);
    if (tOptions.bBenchMode)
    {
        m_optBench.emplace();  // [C++] optional 안에 BenchScript를 그 자리에서 생성 (기본 시나리오)
        std::filesystem::remove(GetBenchCsvPath());  // 지난 벤치 결과 비움 (이번 실행 = 파일 하나)
    }
}

void Game::Run()
{
    while (!WindowShouldClose() && !m_bQuit)
    {
        m_tProfiler.BeginFrame();
        m_tFrameSpatial = {};
        m_tInput.PollDebug(m_tDebugIntent);
        HandleDebugKeys();
        UpdateBench(GetFrameTime());

        m_tInput.PollMenu(m_tMenuIntent);
        if (Entity* pPlayer = m_tWorld.Get(m_hPlayer))
            m_tInput.Poll(pPlayer->tMoveIntent, pPlayer->tAttackIntent);

        // 고정 업데이트: 게임 로직은 항상 kStep(1/60s)만 받음
        const int iStepsThisFrame = m_tTimestep.Advance(GetFrameTime());
        for (int i = 0; i < iStepsThisFrame; ++i)
        {
            if (m_iHitStopTicks > 0)  // 히트스톱: 이 틱은 시뮬레이션 정지 (입력 래치는 그대로 -> 멈춤 끝나고 처리)
            {
                --m_iHitStopTicks;
                continue;
            }
            {
                const ProfileScope tScope(m_tProfiler, ProfileSection::Tick);
                FixedUpdate(FixedTimestep::kStep);
            }
            // 래치된 입력은 첫 틱이 소비 (Flush·방 전환 뒤라 다시 꺼냄)
            m_tMenuIntent.bConfirmPressed = false;
            m_tDebugIntent.iSpawnEnemyPresses = 0;
            m_tDebugIntent.iSpawnBurstPresses = 0;
            if (Entity* pPlayer = m_tWorld.Get(m_hPlayer))
            {
                pPlayer->tMoveIntent.bJumpPressed = false;
                pPlayer->tMoveIntent.bDashPressed = false;
                pPlayer->tAttackIntent.bAttackPressed = false;
            }
        }

        m_tAudio.PlayQueued();  // 이번 프레임 틱들이 요청한 소리 (같은 소리는 1번)
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
            .tDebug = MakeDebugInfo(iStepsThisFrame),
        };
        {
            const ProfileScope tScope(m_tProfiler, ProfileSection::Render);
            m_tRender.Draw(tView);
        }
        m_tRender.Present();  // vsync 대기가 들어 있어서 측정 밖
        m_tProfiler.EndFrame();
        FinishFrameMeasure(iStepsThisFrame);
    }
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
    {
        m_tItems.SpawnClearReward(m_tWorld, m_vLastKillPos);
        m_tAudio.Queue(SfxId::RoomClear);
    }

    // F2(1마리)·F4(100마리) 스트레스 소환 (순회 밖이라 Create 안전)
    SpawnStressEnemies(m_tDebugIntent.iSpawnEnemyPresses + m_tDebugIntent.iSpawnBurstPresses * kStressBurstCount);

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
    // { ProfileScope ... } 블록 = 그 구간 시간 측정 (블록을 나갈 때 소멸자가 기록, measure.md)
    int iVolleys = 0;
    {
        const ProfileScope tScope(m_tProfiler, ProfileSection::Ai);
        m_tEnemyAI.Update(m_tWorld, m_hPlayer, fDt);    // Chaser (사용자)
        m_tShooterAI.Update(m_tWorld, m_hPlayer, fDt);  // Shooter: 발사도 의도만
        m_tBossAI.Update(m_tWorld, m_hPlayer, fDt);     // 보스: 조준 3연사 <-> 8방향 (발사 의도)
        m_tLedgeGuard.Apply(m_tWorld, m_tRoom);         // 발판 끝이면 이동 의도 0 (AI 다음, Motor 전)
        iVolleys = m_tProjectiles.FireFromIntents(m_tWorld, kBulletDef);
    }
    {
        const ProfileScope tScope(m_tProfiler, ProfileSection::Physics);
        m_tWorld.ForEachAlive([&](Handle, Entity& tEntity) {
            if (UsesMotor(tEntity))
                m_tPlayerMotor.Update(tEntity.tMoveIntent, tEntity.tBody, tEntity.tMotor,
                                      GetMoveSpeed(tEntity, m_tPlayerStats), fDt);
        });
        m_tWorld.ForEachAlive([&](Handle, Entity& tEntity) {
            if (tEntity.eType == EntityType::Projectile)
                return;  // 투사체는 직선 이동만 (아래 Projectiles.Update)
            m_tPhysics.Update(tEntity.tBody, m_tRoom, fDt);
            if (tEntity.tMotor.tDash.IsDashing())  // 연출: 대시 잔상 (판정 무관)
                m_tParticles.SpawnGhost(GetBounds(tEntity.tBody));
        });
    }
    {
        const ProfileScope tScope(m_tProfiler, ProfileSection::Projectiles);
        m_vecWallHits.clear();
        m_tProjectiles.Update(m_tWorld, m_tRoom, fDt, m_vecWallHits);
        for (const Vec2& vWallHit : m_vecWallHits)
            m_tParticles.SpawnHitSparks(vWallHit, 1, false, 0.35f);
    }
    {
        const ProfileScope tScope(m_tProfiler, ProfileSection::SpatialCombat);
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
        m_tCombat.ResolveProjectileHits(m_tWorld, m_tSpatial, m_vecHitReports);
    }
    const SpatialQueryStats& tTickSpatial = m_tSpatial.GetStats();  // 이번 틱 검색 비용 -> 프레임 합 (측정)
    m_tFrameSpatial.iQueries += tTickSpatial.iQueries;
    m_tFrameSpatial.llSlotsVisited += tTickSpatial.llSlotsVisited;
    m_tFrameSpatial.llOverlapChecks += tTickSpatial.llOverlapChecks;
    for (const HitReport& tReport : m_vecHitReports)
    {
        m_tParticles.SpawnHitSparks(tReport.vPos, tReport.iDir, tReport.bKilled, tReport.fScale);
        if (tReport.bTargetIsPlayer)
        {
            m_iHitStopTicks = kPlayerDef.iHitStopTicks;  // 플레이어가 맞음 -> 다음 몇 틱 정지 (피격감)
            m_tCamera.AddShake(kPlayerHurtShake);       // 화면 흔들림 (연출)
        }
    }
    HandleKillDrops();
    QueueTickSounds(iVolleys);
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
