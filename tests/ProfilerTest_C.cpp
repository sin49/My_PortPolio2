// 측정기 테스트 (docs/design/measure.md)
#include "core/BenchScript_C.h"
#include "core/MeasureRecorder_C.h"
#include "core/Profiler_C.h"
#include "core/World_U.h"
#include "data/EntityFactory_C.h"
#include "systems/SpatialQuery_C.h"

#include <doctest/doctest.h>

#include <thread>
#include <ostream>
#include <vector>

TEST_CASE("Profiler: 스코프가 끝날 때 걸린 시간을 더함, 한 프레임 안 여러 번은 합산")
{
    FrameProfiler tProfiler;
    tProfiler.BeginFrame();
    for (int i = 0; i < 2; ++i)
    {
        const ProfileScope tScope(tProfiler, ProfileSection::Ai);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    tProfiler.EndFrame();
    CHECK(tProfiler.GetLastFrameMs(ProfileSection::Ai) >= 4.0);
    CHECK(tProfiler.GetLastFrameMs(ProfileSection::Physics) == doctest::Approx(0.0));
    CHECK(tProfiler.GetSmoothedMs(ProfileSection::Ai) == doctest::Approx(tProfiler.GetLastFrameMs(ProfileSection::Ai)));
}

TEST_CASE("Profiler: 평균은 새 값 쪽으로 kSmoothing만큼만 움직임")
{
    FrameProfiler tProfiler;
    tProfiler.BeginFrame();
    tProfiler.Add(ProfileSection::Render, 10.0);
    tProfiler.EndFrame();
    tProfiler.BeginFrame();
    tProfiler.Add(ProfileSection::Render, 20.0);
    tProfiler.EndFrame();
    CHECK(tProfiler.GetSmoothedMs(ProfileSection::Render) == doctest::Approx(10.0 + FrameProfiler::kSmoothing * 10.0));
    CHECK(tProfiler.GetLastFrameMs(ProfileSection::Render) == doctest::Approx(20.0));
}

TEST_CASE("SpatialQuery 측정: 전체 순회는 질문마다 용량만큼 칸을, 살아 있는 수만큼 겹침 검사 / 해시는 후보만. Rebuild로 0")
{
    World tWorld;
    for (int i = 0; i < 5; ++i)
        tWorld.Create(MakePlayerEntity({static_cast<float>(i) * 100.0f, 0.0f}));
    SpatialQuery tSpatial{tWorld};
    tSpatial.SetMode(SpatialMode::FullScan);
    std::vector<Handle> vecOut;
    tSpatial.Rebuild();
    tSpatial.QueryNearby(Rect{0.0f, 0.0f, 10.0f, 10.0f}, vecOut);
    tSpatial.QueryNearby(Rect{0.0f, 0.0f, 10.0f, 10.0f}, vecOut);
    CHECK(tSpatial.GetStats().iQueries == 2);
    CHECK(tSpatial.GetStats().llSlotsVisited == 2LL * World::kCapacity);
    CHECK(tSpatial.GetStats().llOverlapChecks == 10);
    tSpatial.Rebuild();
    CHECK(tSpatial.GetStats().iQueries == 0);

    // 해시: (0,0) 근처 칸의 후보만 검사 (100px 간격 5개 중 0번만 같은 칸 근처)
    tSpatial.SetMode(SpatialMode::Hash);
    tSpatial.Rebuild();
    tSpatial.QueryNearby(Rect{0.0f, 0.0f, 10.0f, 10.0f}, vecOut);
    CHECK(tSpatial.GetStats().iQueries == 1);
    CHECK(tSpatial.GetStats().llOverlapChecks < 5);
    CHECK(vecOut.size() == 1);
}

TEST_CASE("Profiler: 프레임당 합과 별개로 1번당(틱당) 평균 -> 느린 프레임에 틱이 많이 돌아도 안 부풀음")
{
    FrameProfiler tProfiler;
    tProfiler.BeginFrame();
    for (int i = 0; i < 3; ++i)  // 느린 프레임: 틱 3번
        tProfiler.Add(ProfileSection::Tick, 2.0);
    tProfiler.EndFrame();
    CHECK(tProfiler.GetLastFrameMs(ProfileSection::Tick) == doctest::Approx(6.0));
    CHECK(tProfiler.GetLastFrameCalls(ProfileSection::Tick) == 3);
    CHECK(tProfiler.GetSmoothedPerCallMs(ProfileSection::Tick) == doctest::Approx(2.0));

    tProfiler.BeginFrame();  // 틱 0번인 프레임: 틱당 평균은 그대로
    tProfiler.EndFrame();
    CHECK(tProfiler.GetSmoothedPerCallMs(ProfileSection::Tick) == doctest::Approx(2.0));
}

TEST_CASE("MeasureRecorder: 구간 시간이 차면 끝, 틱 구간은 틱당·render는 프레임당 평균")
{
    MeasureRecorder tRecorder;
    CHECK_FALSE(tRecorder.AddFrame({}));  // 시작 전엔 무시
    tRecorder.Start("spread", "full-scan", 1000, 0.1);  // 100ms
    FrameSample tSample = {.dFrameMs = 25.0, .iTicks = 2, .llQueries = 10, .llOverlapChecks = 400, .iEntities = 1001};
    tSample.arrSectionMs[static_cast<std::size_t>(ProfileSection::SpatialCombat)] = 8.0;  // 프레임당 합 (틱 2번)
    tSample.arrSectionMs[static_cast<std::size_t>(ProfileSection::Render)] = 5.0;
    CHECK_FALSE(tRecorder.AddFrame(tSample));
    CHECK_FALSE(tRecorder.AddFrame(tSample));
    CHECK_FALSE(tRecorder.AddFrame(tSample));
    CHECK(tRecorder.AddFrame(tSample));  // 4프레임 = 100ms
    CHECK_FALSE(tRecorder.IsRecording());

    const MeasureRow& tRow = tRecorder.GetResult();
    CHECK(tRow.strScene == "spread");
    CHECK(tRow.iFrames == 4);
    CHECK(tRow.iTicks == 8);
    CHECK(tRow.iEntities == 1001);
    CHECK(tRow.dFps == doctest::Approx(40.0));
    CHECK(tRow.arrPerUnitMs[static_cast<std::size_t>(ProfileSection::SpatialCombat)] == doctest::Approx(4.0));
    CHECK(tRow.arrPerUnitMs[static_cast<std::size_t>(ProfileSection::Render)] == doctest::Approx(5.0));
    CHECK(tRow.dQueriesPerTick == doctest::Approx(5.0));
    CHECK(tRow.dChecksPerTick == doctest::Approx(200.0));
}

TEST_CASE("BenchScript: 단계마다 새 게임 -> 소환 -> 퍼짐 측정 -> 몰림 측정, 끝나면 종료")
{
    BenchScript tBench({.vecCounts = {100, 200}, .dSpreadDelaySec = 0.5, .dClusterDelaySec = 2.0, .dRecordSec = 1.0});
    std::vector<BenchCommand> vecLog;
    bool bBusy = false;
    double dBusyLeft = 0.0;
    for (int i = 0; i < 200 && !tBench.IsFinished(); ++i)  // 0.1초 프레임
    {
        dBusyLeft -= 0.1;
        bBusy = dBusyLeft > 0.0;
        const BenchCommand tCommand = tBench.Update(0.1, bBusy);
        if (tCommand.bStartRecord)
            dBusyLeft = tCommand.dRecordSec;
        if (tCommand.bResetGame || tCommand.iSpawnCount > 0 || tCommand.bStartRecord)
            vecLog.push_back(tCommand);
    }
    REQUIRE(tBench.IsFinished());
    REQUIRE(vecLog.size() == 8);  // 단계 2개 × (리셋, 소환, 퍼짐, 몰림)
    CHECK(vecLog[0].bResetGame);
    CHECK(vecLog[1].iSpawnCount == 100);
    CHECK(vecLog[2].svScene == "spread");
    CHECK(vecLog[3].svScene == "cluster");
    CHECK(vecLog[3].iTarget == 100);
    CHECK(vecLog[5].iSpawnCount == 200);
    CHECK(tBench.Update(0.1, false).bFinished);
}
