#include "core/MeasureRecorder_C.h"

#include <fstream>

void MeasureRecorder::Start(std::string_view svScene, std::string_view svSpatial, int iTarget, double dDurationSec)
{
    m_bRecording = true;
    m_dDurationSec = dDurationSec;
    m_dElapsedMs = 0.0;
    m_arrSumMs = {};
    m_llQueries = 0;
    m_llSlots = 0;
    m_llChecks = 0;
    m_tResult = {};
    m_tResult.strScene = svScene;
    m_tResult.strSpatial = svSpatial;
    m_tResult.iTarget = iTarget;
}

bool MeasureRecorder::AddFrame(const FrameSample& tSample)
{
    if (!m_bRecording)
        return false;

    m_dElapsedMs += tSample.dFrameMs;
    ++m_tResult.iFrames;
    m_tResult.iTicks += tSample.iTicks;
    m_tResult.iEntities = tSample.iEntities;
    for (std::size_t i = 0; i < kProfileSectionCount; ++i)
        m_arrSumMs[i] += tSample.arrSectionMs[i];
    m_llQueries += tSample.llQueries;
    m_llSlots += tSample.llSlotsVisited;
    m_llChecks += tSample.llOverlapChecks;

    if (m_dElapsedMs < m_dDurationSec * 1000.0)
        return false;

    // 구간 끝 -> 평균
    const double dFrames = static_cast<double>(m_tResult.iFrames);
    const double dTicks = m_tResult.iTicks > 0 ? static_cast<double>(m_tResult.iTicks) : 1.0;
    m_tResult.dFrameMs = m_dElapsedMs / dFrames;
    m_tResult.dFps = dFrames / (m_dElapsedMs / 1000.0);
    for (std::size_t i = 0; i < kProfileSectionCount; ++i)
    {
        const bool bPerFrame = static_cast<ProfileSection>(i) == ProfileSection::Render;
        m_tResult.arrPerUnitMs[i] = m_arrSumMs[i] / (bPerFrame ? dFrames : dTicks);
    }
    m_tResult.dQueriesPerTick = static_cast<double>(m_llQueries) / dTicks;
    m_tResult.dSlotsPerTick = static_cast<double>(m_llSlots) / dTicks;
    m_tResult.dChecksPerTick = static_cast<double>(m_llChecks) / dTicks;
    m_bRecording = false;
    return true;
}

bool MeasureRecorder::AppendCsv(const std::filesystem::path& tPath, const MeasureRow& tRow, std::string_view svBuild,
                                int iRoom)
{
    const bool bNewFile = !std::filesystem::exists(tPath);
    std::ofstream tOut(tPath, std::ios::app);
    if (!tOut)
        return false;
    if (bNewFile)
    {
        tOut << "build,spatial,scene,room,target,entities,frames,ticks,fps,frame_ms";
        for (std::size_t i = 0; i < kProfileSectionCount; ++i)
        {
            const bool bPerFrame = static_cast<ProfileSection>(i) == ProfileSection::Render;
            tOut << ',' << GetProfileSectionName(static_cast<ProfileSection>(i)) << (bPerFrame ? "_ms_per_frame" : "_ms_per_tick");
        }
        tOut << ",queries_per_tick,slots_per_tick,overlap_checks_per_tick\n";
    }
    tOut << svBuild << ',' << tRow.strSpatial << ',' << tRow.strScene << ',' << iRoom << ',' << tRow.iTarget << ','
         << tRow.iEntities << ',' << tRow.iFrames << ',' << tRow.iTicks << ',' << tRow.dFps << ',' << tRow.dFrameMs;
    for (const double dMs : tRow.arrPerUnitMs)
        tOut << ',' << dMs;
    tOut << ',' << tRow.dQueriesPerTick << ',' << tRow.dSlotsPerTick << ',' << tRow.dChecksPerTick << '\n';
    return true;
}
