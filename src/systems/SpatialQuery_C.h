#pragma once

#include "core/Handle_U.h"
#include "core/Rect_C.h"
#include "systems/SpatialHash_U.h"

#include <vector>

class World;

// 틱당 검색 비용 (측정용, docs/design/measure.md). Rebuild 때 0으로
struct SpatialQueryStats
{
    int iQueries = 0;               // QueryNearby 호출 수
    long long llSlotsVisited = 0;   // 훑은 World 칸 수 (전체 순회 = 질문마다 용량만큼 / 해시 = 후보 수)
    long long llOverlapChecks = 0;  // 정확한 겹침 검사 수 (전체 순회 = 살아 있는 전부, 해시 = 후보만)
};

// 검색 방식 (측정 비교용으로 실행 중 전환 가능, F3)
enum class SpatialMode
{
    FullScan,  // MVP: World 전체 순회
    Hash,      // 사용자 SpatialHash로 후보만 -> 정확한 겹침 검사 (기본)
};

// 주변 엔티티 검색의 유일한 통로 (CLAUDE.md). Combat·AI는 이 인터페이스만 앎.
// 내부 방식(전체 순회 / 해시)이 바뀌어도 호출부와 결과는 같음 (테스트로 확인)
class SpatialQuery
{
public:
    explicit SpatialQuery(const World& tWorld) : m_tWorld(tWorld) {}

    // 매 틱 Physics 직후 호출 (위치가 확정된 뒤). 측정값을 비우고, 해시 모드면 격자를 다시 채움
    void Rebuild();

    // rcArea와 몸이 겹치는 살아 있는(삭제 예약 포함) 엔티티. 핸들 번호 순. 결과는 호출자 버퍼에 (재사용 -> 할당 없음)
    void QueryNearby(const Rect& rcArea, std::vector<Handle>& vecOut) const;

    void SetMode(SpatialMode eMode) { m_eMode = eMode; }  // 다음 Rebuild부터 적용
    SpatialMode GetMode() const { return m_eMode; }
    const char* GetModeName() const;  // 측정 기록용 "full-scan" / "hash"
    // 해시 칸 크기 (px, 측정 실험용 --cell=N). 격자를 새로 만듦 -> 다음 Rebuild부터
    void SetCellSize(float fCellSize);
    float GetCellSize() const { return m_fCellSize; }

    const SpatialQueryStats& GetStats() const { return m_tStats; }  // 지난 Rebuild 이후 누적

private:
    const World& m_tWorld;  // 빌려 씀 (Game이 World와 SpatialQuery를 함께 소유, 수명 같음)
    SpatialMode m_eMode = SpatialMode::Hash;
    SpatialMode m_eBuiltMode = SpatialMode::FullScan;  // 지난 Rebuild가 어떤 방식이었나 (모드 전환 직후 안전)
    float m_fCellSize = SpatialHash::kDefaultCellSize;
    SpatialHash m_tHash{m_fCellSize};
    // [C++] mutable: const 함수(QueryNearby) 안에서도 바꿀 수 있는 멤버. 검색 "결과"는 안 바꾸고 세기·임시 버퍼만
    //       C#엔 const 함수 개념이 없어서 해당 없음
    mutable SpatialQueryStats m_tStats;
    mutable std::vector<Handle> m_vecCandidates;  // 해시 후보 버퍼 재사용
};
