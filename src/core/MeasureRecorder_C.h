#pragma once
// 측정 구간(몇 초) 평균 -> CSV 한 줄 (docs/design/measure.md 6장). 순간 1프레임 값 대신 평균이라 안정적.
// raylib 모름 -> 테스트 가능. 프레임마다 Game이 FrameSample을 만들어 넘김
#include "core/Profiler_C.h"

#include <array>
#include <filesystem>
#include <string>
#include <string_view>

// 한 프레임 측정값
struct FrameSample
{
    double dFrameMs = 0.0;  // 실제 프레임 시간 (vsync 포함 = 체감 FPS)
    int iTicks = 0;         // 이번 프레임에 돈 고정 틱 수
    std::array<double, kProfileSectionCount> arrSectionMs = {};  // 프레임당 합 (FrameProfiler 마지막 프레임)
    long long llQueries = 0;        // 이번 프레임 틱들의 검색 합
    long long llSlotsVisited = 0;
    long long llOverlapChecks = 0;
    int iEntities = 0;
};

// 구간 평균 결과 = CSV 한 줄
struct MeasureRow
{
    std::string strScene;  // 예: "spread", "cluster", "manual"
    std::string strSpatial = "full-scan";
    int iTarget = 0;    // 목표 적 수 (벤치 단계)
    int iEntities = 0;  // 구간 끝의 실제 엔티티 수
    int iFrames = 0;
    int iTicks = 0;
    double dFps = 0.0;      // 프레임 수 / 걸린 시간
    double dFrameMs = 0.0;  // 프레임 평균
    // Render만 프레임당 평균, 나머지는 **틱당** 평균 (프레임당 틱 수에 안 흔들림)
    std::array<double, kProfileSectionCount> arrPerUnitMs = {};
    double dQueriesPerTick = 0.0;
    double dSlotsPerTick = 0.0;
    double dChecksPerTick = 0.0;
};

inline constexpr const char* kManualCsvPath = "stress_results.csv";  // F5: 실행 폴더에 계속 쌓임
// --bench 결과: bench_results_<검색 방식>.csv (한 번 = 파일 하나, 시작할 때 비움). GameDebug_C.cpp가 만듦

class MeasureRecorder
{
public:
    void Start(std::string_view svScene, std::string_view svSpatial, int iTarget, double dDurationSec = 3.0);
    bool IsRecording() const { return m_bRecording; }
    double GetElapsedSec() const { return m_dElapsedMs / 1000.0; }
    double GetDurationSec() const { return m_dDurationSec; }

    // 기록 중일 때만 쌓음. 구간이 이번 프레임으로 끝나면 true (결과는 GetResult)
    bool AddFrame(const FrameSample& tSample);
    const MeasureRow& GetResult() const { return m_tResult; }

    // CSV 파일 끝에 한 줄 (없으면 머리줄부터). svBuild: "Release"/"Debug", iRoom: 방 번호
    static bool AppendCsv(const std::filesystem::path& tPath, const MeasureRow& tRow, std::string_view svBuild,
                          int iRoom);

private:
    bool m_bRecording = false;
    double m_dDurationSec = 3.0;
    double m_dElapsedMs = 0.0;
    std::array<double, kProfileSectionCount> m_arrSumMs = {};
    long long m_llQueries = 0;
    long long m_llSlots = 0;
    long long m_llChecks = 0;
    MeasureRow m_tResult;
};
