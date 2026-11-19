// Game의 측정·디버그 부분 (docs/design/measure.md). Game_C.cpp가 300줄을 넘지 않게 나눔 (같은 클래스, 파일만 분리)
#include "core/Game_C.h"

#include "raylib.h"

namespace
{
constexpr double kManualRecordSec = 3.0;  // F5: 3초 평균

#ifdef NDEBUG  // 표준 매크로 (Release = NDEBUG 정의). 수치가 어느 빌드인지 기록에 남기려고
constexpr const char* kBuildName = "Release";
#else
constexpr const char* kBuildName = "Debug";
#endif

} // namespace

void Game::HandleDebugKeys()
{
    if (m_tDebugIntent.bToggleDebugPressed)  // 디버그 모드 = 패널 + 플레이어 무적 (사용자 결정)
    {
        m_bShowDebug = !m_bShowDebug;
        m_tCombat.SetPlayerInvincible(m_bShowDebug);
    }
    m_tDebugIntent.bToggleDebugPressed = false;

    if (m_tDebugIntent.bRecordPressed && !m_tRecorder.IsRecording())
        m_tRecorder.Start("manual", GetSpatialLabel(), m_tWorld.GetAliveCount(), kManualRecordSec);
    m_tDebugIntent.bRecordPressed = false;

    if (m_tDebugIntent.bToggleSpatialPressed)  // F3: 같은 장면에서 검색 방식만 바꿔 비교 (다음 틱 Rebuild부터)
        m_tSpatial.SetMode(m_tSpatial.GetMode() == SpatialMode::Hash ? SpatialMode::FullScan : SpatialMode::Hash);
    m_tDebugIntent.bToggleSpatialPressed = false;
}

void Game::UpdateBench(float fFrameSec)
{
    if (!m_optBench)
        return;

    const BenchCommand tCommand = m_optBench->Update(fFrameSec, m_tRecorder.IsRecording());
    if (tCommand.bResetGame)
    {
        m_tRoomFlow.StartNewGame(m_tWorld, m_hPlayer);  // 튜토리얼 방부터, 적 초기화
        m_bShowDebug = true;
        m_tCombat.SetPlayerInvincible(true);
    }
    SpawnStressEnemies(tCommand.iSpawnCount);
    if (tCommand.bStartRecord)
        m_tRecorder.Start(tCommand.svScene, GetSpatialLabel(), tCommand.iTarget, tCommand.dRecordSec);
    if (tCommand.bFinished)
        m_bQuit = true;
}

void Game::SpawnStressEnemies(int iCount)
{
    if (iCount <= 0 || m_tRoomFlow.GetPhase() != GamePhase::Playing ||
        m_tRoomFlow.GetRoomState() != RoomState::Locked)
        return;  // 클리어 안 된 방에서만 (사용자 조건)
    for (int i = 0; i < iCount && m_tWorld.GetAliveCount() < static_cast<int>(World::kCapacity); ++i)
        m_tStressSpawner.SpawnSmallChaser(m_tWorld, m_tRoom, m_hPlayer);
}

void Game::FinishFrameMeasure(int iTicks)
{
    if (!m_tRecorder.IsRecording())
        return;
    FrameSample tSample = {
        .dFrameMs = static_cast<double>(GetFrameTime()) * 1000.0,  // Present 뒤 = 방금 끝난 프레임 (vsync 포함)
        .iTicks = iTicks,
        .llQueries = m_tFrameSpatial.iQueries,
        .llSlotsVisited = m_tFrameSpatial.llSlotsVisited,
        .llOverlapChecks = m_tFrameSpatial.llOverlapChecks,
        .iEntities = m_tWorld.GetAliveCount(),
    };
    for (std::size_t i = 0; i < kProfileSectionCount; ++i)
        tSample.arrSectionMs[i] = m_tProfiler.GetLastFrameMs(static_cast<ProfileSection>(i));

    if (!m_tRecorder.AddFrame(tSample))
        return;
    MeasureRecorder::AppendCsv(m_optBench ? GetBenchCsvPath() : kManualCsvPath, m_tRecorder.GetResult(), kBuildName,
                               m_tRoomFlow.GetRoomIndex());
}

DebugInfo Game::MakeDebugInfo(int iTicks) const
{
    return DebugInfo{
        .iStepsThisFrame = iTicks,
        .llTotalTicks = m_llTotalTicks,
        .bShow = m_bShowDebug,
        .bPlayerInvincible = m_tCombat.IsPlayerInvincible(),
        .pProfiler = &m_tProfiler,
        .tSpatialStats = m_tSpatial.GetStats(),
        .szSpatialMode = m_tSpatial.GetModeName(),
        .bRecording = m_tRecorder.IsRecording(),
        .dRecordElapsedSec = m_tRecorder.GetElapsedSec(),
        .dRecordDurationSec = m_tRecorder.GetDurationSec(),
        .iBenchStep = m_optBench ? m_optBench->GetStepIndex() : -1,
        .iBenchStepCount = m_optBench ? m_optBench->GetStepCount() : 0,
    };
}

std::string Game::GetSpatialLabel() const
{
    if (m_tSpatial.GetMode() == SpatialMode::FullScan)
        return "full-scan";
    return "hash-" + std::to_string(static_cast<int>(m_tSpatial.GetCellSize()));
}

std::string Game::GetBenchCsvPath() const
{
    return "bench_results_" + GetSpatialLabel() + ".csv";
}
