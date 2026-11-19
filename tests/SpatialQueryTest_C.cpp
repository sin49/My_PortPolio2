// SpatialQuery: 전체 순회와 해시가 **같은 결과**인지 (docs/design/spatial-hash.md 5장)
// "빠른데 틀린" 최적화를 막는 테스트. 결과 순서(핸들 번호 순)까지 같아야 Combat 동작이 안 바뀜
#include "core/World_U.h"
#include "data/EntityFactory_C.h"
#include "systems/SpatialQuery_C.h"

#include <doctest/doctest.h>

#include <random>
#include <vector>

namespace
{
std::vector<Handle> QueryWith(SpatialQuery& tSpatial, SpatialMode eMode, const Rect& rcArea)
{
    tSpatial.SetMode(eMode);
    tSpatial.Rebuild();
    std::vector<Handle> vecOut;
    tSpatial.QueryNearby(rcArea, vecOut);
    return vecOut;
}
} // namespace

TEST_CASE("SpatialQuery: 전체 순회 == 해시 (랜덤 몸 500개 + 음수 좌표 + 삭제 예약, 질문 300번, 칸 크기별)")
{
    for (const float fCellSize : {32.0f, 64.0f, 128.0f, 256.0f})  // 칸 크기 실험값 전부 (measure.md 10장)
    {
        CAPTURE(fCellSize);
        World tWorld;
        std::mt19937 tRng(1119u);
        std::uniform_real_distribution<float> tPos(-200.0f, 3000.0f);
        std::uniform_real_distribution<float> tSize(4.0f, 90.0f);
        std::vector<Handle> vecAll;
        for (int i = 0; i < 500; ++i)
        {
            Entity tEntity = MakePlayerEntity({tPos(tRng), tPos(tRng) * 0.3f});
            tEntity.tBody.vSize = {tSize(tRng), tSize(tRng)};
            vecAll.push_back(tWorld.Create(tEntity));
        }
        for (std::size_t i = 0; i < vecAll.size(); i += 7)
            tWorld.Destroy(vecAll[i]);  // 삭제 예약 중인 것도 틱 안에선 검색됨 (둘 다 같아야 함)

        SpatialQuery tSpatial{tWorld};
        tSpatial.SetCellSize(fCellSize);
        std::uniform_real_distribution<float> tAreaSize(1.0f, 400.0f);
        int iNonEmpty = 0;
        for (int i = 0; i < 300; ++i)
        {
            const Rect rcArea = {tPos(tRng), tPos(tRng) * 0.3f, tAreaSize(tRng), tAreaSize(tRng)};
            const std::vector<Handle> vecFull = QueryWith(tSpatial, SpatialMode::FullScan, rcArea);
            const std::vector<Handle> vecHash = QueryWith(tSpatial, SpatialMode::Hash, rcArea);
            CAPTURE(i);
            REQUIRE(vecFull == vecHash);
            if (!vecFull.empty())
                ++iNonEmpty;
        }
        CHECK(iNonEmpty > 50);  // 대부분 빈 결과면 의미 없는 테스트
    }
}

TEST_CASE("SpatialQuery: 해시는 Rebuild 전 위치로 찾음 -> 매 틱 위치 확정 뒤 Rebuild (Game 순서)")
{
    World tWorld;
    const Handle hEntity = tWorld.Create(MakePlayerEntity({0.0f, 0.0f}));
    SpatialQuery tSpatial{tWorld};
    std::vector<Handle> vecOut;

    tSpatial.Rebuild();  // 기본 = 해시
    REQUIRE(tSpatial.GetMode() == SpatialMode::Hash);
    tWorld.Get(hEntity)->tBody.vPos = {1000.0f, 0.0f};
    tSpatial.QueryNearby(Rect{1000.0f, 0.0f, 10.0f, 10.0f}, vecOut);
    CHECK(vecOut.empty());  // 옮긴 뒤 Rebuild 안 함 -> 못 찾음 (그래서 Physics 다음에 Rebuild)

    tSpatial.Rebuild();
    tSpatial.QueryNearby(Rect{1000.0f, 0.0f, 10.0f, 10.0f}, vecOut);
    CHECK(vecOut.size() == 1);
}
