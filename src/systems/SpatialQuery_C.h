#pragma once

#include "core/Handle_U.h"
#include "core/Rect_C.h"

#include <vector>

class World;

// 주변 엔티티 검색의 유일한 통로 (CLAUDE.md). Combat·AI는 이 인터페이스만 앎.
// MVP: World 전체 순회. 금요일에 내부만 Spatial Hash로 바꿔도 호출부는 그대로.
class SpatialQuery
{
public:
    explicit SpatialQuery(const World& tWorld) : m_tWorld(tWorld) {}

    // 매 틱 Physics 직후 호출 (위치가 확정된 뒤). MVP는 할 일 없음, 해시 때 격자 재구축.
    void Rebuild() {}

    // rcArea와 몸이 겹치는 살아 있는(삭제 예약 포함) 엔티티. 결과는 호출자 버퍼에 (재사용 -> 할당 없음)
    void QueryNearby(const Rect& rcArea, std::vector<Handle>& vecOut) const;

private:
    const World& m_tWorld;  // 빌려 씀 (Game이 World와 SpatialQuery를 함께 소유, 수명 같음)
};
