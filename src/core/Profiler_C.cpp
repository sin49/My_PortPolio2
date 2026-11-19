#include "core/Profiler_C.h"

void FrameProfiler::BeginFrame()
{
    m_arrFrameMs = {};
    m_arrFrameCalls = {};
}

void FrameProfiler::Add(ProfileSection eSection, double dMs)
{
    m_arrFrameMs[ToIndex(eSection)] += dMs;
    ++m_arrFrameCalls[ToIndex(eSection)];
}

void FrameProfiler::EndFrame()
{
    m_arrLastMs = m_arrFrameMs;
    m_arrLastCalls = m_arrFrameCalls;
    for (std::size_t i = 0; i < kProfileSectionCount; ++i)
    {
        m_arrSmoothedMs[i] =
            m_bHasSample ? m_arrSmoothedMs[i] + kSmoothing * (m_arrFrameMs[i] - m_arrSmoothedMs[i]) : m_arrFrameMs[i];
        if (m_arrFrameCalls[i] == 0)
            continue;  // 틱이 0번인 프레임: 틱당 평균은 그대로 둠
        const double dPerCall = m_arrFrameMs[i] / static_cast<double>(m_arrFrameCalls[i]);
        m_arrSmoothedPerCallMs[i] = m_arrHasPerCall[i]
                                        ? m_arrSmoothedPerCallMs[i] + kSmoothing * (dPerCall - m_arrSmoothedPerCallMs[i])
                                        : dPerCall;
        m_arrHasPerCall[i] = true;
    }
    m_bHasSample = true;
}

const char* GetProfileSectionName(ProfileSection eSection)
{
    switch (eSection)
    {
    case ProfileSection::Tick:
        return "tick";
    case ProfileSection::Ai:
        return "ai";
    case ProfileSection::Physics:
        return "physics";
    case ProfileSection::Projectiles:
        return "projectiles";
    case ProfileSection::SpatialCombat:
        return "spatial_combat";
    case ProfileSection::Render:
        return "render";
    case ProfileSection::Count:
        break;
    }
    return "";
}
