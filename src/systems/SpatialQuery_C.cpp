#include "systems/SpatialQuery_C.h"

#include "core/World_U.h"

void SpatialQuery::Rebuild()
{
    m_tStats = {};
    m_eBuiltMode = m_eMode;
    if (m_eMode != SpatialMode::Hash)
        return;
    // 격자 다시 채우기: 칸 vector는 비우기만 (Clear) -> 몇 틱 뒤로는 할당 없음
    m_tHash.Clear();
    m_tWorld.ForEachAlive([&](Handle hEntity, const Entity& tEntity) { m_tHash.Insert(hEntity, GetBounds(tEntity.tBody)); });
}

void SpatialQuery::QueryNearby(const Rect& rcArea, std::vector<Handle>& vecOut) const
{
    vecOut.clear();
    ++m_tStats.iQueries;

    if (m_eBuiltMode == SpatialMode::FullScan)
    {
        m_tStats.llSlotsVisited += World::kCapacity;  // ForEachAlive는 빈 칸까지 전부 훑음
        m_tWorld.ForEachAlive([&](Handle hEntity, const Entity& tEntity) {
            ++m_tStats.llOverlapChecks;
            if (Overlaps(rcArea, GetBounds(tEntity.tBody)))
                vecOut.push_back(hEntity);
        });
        return;
    }

    // 해시: 걸친 칸의 후보만 (핸들 번호 순, 중복 없음) -> 정확한 겹침 검사는 여기서
    m_tHash.Query(rcArea, m_vecCandidates);
    m_tStats.llSlotsVisited += static_cast<long long>(m_vecCandidates.size());
    for (const Handle hCandidate : m_vecCandidates)
    {
        const Entity* pEntity = m_tWorld.Get(hCandidate);
        if (pEntity == nullptr)
            continue;  // Rebuild 뒤에 사라진 것 (틱 중엔 Flush가 없어서 보통 없음)
        ++m_tStats.llOverlapChecks;
        if (Overlaps(rcArea, GetBounds(pEntity->tBody)))
            vecOut.push_back(hCandidate);
    }
}

void SpatialQuery::SetCellSize(float fCellSize)
{
    m_fCellSize = fCellSize;
    m_tHash = SpatialHash(fCellSize);  // [C++] 새 객체를 만들어 이동 대입 (옛 칸 목록은 이때 해제)
    m_eBuiltMode = SpatialMode::FullScan;  // 다음 Rebuild 전까지는 안전하게 전체 순회
}

const char* SpatialQuery::GetModeName() const
{
    return m_eMode == SpatialMode::Hash ? "hash" : "full-scan";
}
