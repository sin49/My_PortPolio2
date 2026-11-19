#include "core/BenchScript_C.h"

#include <utility>

BenchScript::BenchScript(BenchConfig tConfig)
    : m_tConfig(std::move(tConfig))
{
    if (m_tConfig.vecCounts.empty())
        m_eStep = BenchStep::Done;
}

BenchCommand BenchScript::Update(double dFrameSec, bool bRecorderBusy)
{
    BenchCommand tCommand;
    const int iTarget = m_eStep == BenchStep::Done ? 0 : m_tConfig.vecCounts[m_uLevel];
    m_dSinceSpawnSec += dFrameSec;

    switch (m_eStep)
    {
    case BenchStep::Reset:
        tCommand.bResetGame = true;
        m_eStep = BenchStep::Spawn;
        break;
    case BenchStep::Spawn:
        tCommand.iSpawnCount = iTarget;
        m_dSinceSpawnSec = 0.0;
        m_eStep = BenchStep::WaitSpread;
        break;
    case BenchStep::WaitSpread:
        if (m_dSinceSpawnSec >= m_tConfig.dSpreadDelaySec)
        {
            tCommand.bStartRecord = true;
            tCommand.svScene = "spread";
            tCommand.iTarget = iTarget;
            tCommand.dRecordSec = m_tConfig.dRecordSec;
            m_eStep = BenchStep::RecordSpread;
        }
        break;
    case BenchStep::RecordSpread:
        if (!bRecorderBusy)
            m_eStep = BenchStep::WaitCluster;
        break;
    case BenchStep::WaitCluster:
        if (m_dSinceSpawnSec >= m_tConfig.dClusterDelaySec)
        {
            tCommand.bStartRecord = true;
            tCommand.svScene = "cluster";
            tCommand.iTarget = iTarget;
            tCommand.dRecordSec = m_tConfig.dRecordSec;
            m_eStep = BenchStep::RecordCluster;
        }
        break;
    case BenchStep::RecordCluster:
        if (!bRecorderBusy)
        {
            ++m_uLevel;
            m_eStep = m_uLevel < m_tConfig.vecCounts.size() ? BenchStep::Reset : BenchStep::Done;
        }
        break;
    case BenchStep::Done:
        tCommand.bFinished = true;
        break;
    }
    return tCommand;
}
