// SpatialHash 스펙 테스트 (AI 작성). 사용자가 구현한 SpatialHash가 이 테스트를 통과하면 완료.
// 기준: docs/design/spatial-hash.md 4장. 테스트가 설계와 다르다고 생각되면 고치지 말고 먼저 논의.
#include "systems/SpatialHash_U.h"

#include <algorithm>
#include <random>
#include <vector>

#include <doctest/doctest.h>

namespace
{
Handle MakeHandle(std::uint32_t uIndex)
{
    return Handle{.uIndex = uIndex, .uGen = 1};
}

bool Contains(const std::vector<Handle>& vecHandles, Handle hTarget)
{
    return std::find(vecHandles.begin(), vecHandles.end(), hTarget) != vecHandles.end();
}

// 6·7번: 랜덤 사각형 vs 전체 순회. 겹치는 건 하나도 빠지면 안 됨
void CheckNoMissAgainstBruteForce(float fCellSize)
{
    SpatialHash tHash(fCellSize);
    std::mt19937 tRng(1234u);
    std::uniform_real_distribution<float> tPos(-100.0f, 3800.0f);
    std::uniform_real_distribution<float> tSize(8.0f, 90.0f);

    std::vector<Rect> vecRects;
    for (std::uint32_t i = 0; i < 300; ++i)
    {
        vecRects.push_back(Rect{tPos(tRng), tPos(tRng) * 0.25f, tSize(tRng), tSize(tRng)});
        tHash.Insert(MakeHandle(i), vecRects.back());
    }

    std::vector<Handle> vecOut;
    for (int iQuery = 0; iQuery < 100; ++iQuery)
    {
        const Rect rcArea = {tPos(tRng), tPos(tRng) * 0.25f, tSize(tRng) * 2.0f, tSize(tRng) * 2.0f};
        tHash.Query(rcArea, vecOut);
        for (std::uint32_t i = 0; i < vecRects.size(); ++i)
            if (Overlaps(rcArea, vecRects[i]))
                REQUIRE(Contains(vecOut, MakeHandle(i)));
    }
}
} // namespace

TEST_CASE("SpatialHash: 비어 있으면 결과 없음")
{
    const SpatialHash tHash;
    std::vector<Handle> vecOut = {MakeHandle(99)};  // 원래 들어 있던 건 지워져야 함
    tHash.Query(Rect{0.0f, 0.0f, 500.0f, 500.0f}, vecOut);
    CHECK(vecOut.empty());
}

TEST_CASE("SpatialHash: 겹치는 영역에선 나오고 먼 영역에선 안 나옴")
{
    SpatialHash tHash;
    tHash.Insert(MakeHandle(1), Rect{10.0f, 10.0f, 20.0f, 20.0f});
    std::vector<Handle> vecOut;

    tHash.Query(Rect{0.0f, 0.0f, 40.0f, 40.0f}, vecOut);
    REQUIRE(vecOut.size() == 1);
    CHECK(vecOut[0] == MakeHandle(1));

    tHash.Query(Rect{2000.0f, 2000.0f, 40.0f, 40.0f}, vecOut);
    CHECK(vecOut.empty());
}

TEST_CASE("SpatialHash: 칸 여러 개에 걸친 엔티티는 1번만")
{
    SpatialHash tHash(SpatialHash::kDefaultCellSize);
    const float fCell = SpatialHash::kDefaultCellSize;
    // 칸 경계 교차점에 걸쳐서 4칸에 들어감
    tHash.Insert(MakeHandle(7), Rect{fCell - 10.0f, fCell - 10.0f, 20.0f, 20.0f});
    std::vector<Handle> vecOut;
    tHash.Query(Rect{0.0f, 0.0f, fCell * 2.0f, fCell * 2.0f}, vecOut);
    CHECK(vecOut.size() == 1);
}

TEST_CASE("SpatialHash: Clear 뒤엔 안 나오고, 다시 넣으면 나옴")
{
    SpatialHash tHash;
    std::vector<Handle> vecOut;
    for (int iTick = 0; iTick < 3; ++iTick)  // 매 틱 Rebuild 흉내
    {
        tHash.Clear();
        tHash.Query(Rect{0.0f, 0.0f, 100.0f, 100.0f}, vecOut);
        CHECK(vecOut.empty());
        tHash.Insert(MakeHandle(3), Rect{50.0f, 50.0f, 10.0f, 10.0f});
        tHash.Query(Rect{0.0f, 0.0f, 100.0f, 100.0f}, vecOut);
        CHECK(vecOut.size() == 1);
    }
}

TEST_CASE("SpatialHash: 음수 좌표는 0번 칸과 섞이지 않음")
{
    SpatialHash tHash(100.0f);
    tHash.Insert(MakeHandle(1), Rect{-60.0f, 10.0f, 10.0f, 10.0f});  // x -60 ~ -50 = -1번 칸만
    std::vector<Handle> vecOut;
    tHash.Query(Rect{10.0f, 10.0f, 10.0f, 10.0f}, vecOut);  // 0번 칸만
    CHECK(vecOut.empty());
    tHash.Query(Rect{-90.0f, 10.0f, 10.0f, 10.0f}, vecOut);  // -1번 칸
    CHECK(vecOut.size() == 1);
}

TEST_CASE("SpatialHash: 전체 순회로 찾은 겹침은 전부 후보에 있음 (빠뜨림 없음)")
{
    CheckNoMissAgainstBruteForce(SpatialHash::kDefaultCellSize);
}

TEST_CASE("SpatialHash: 칸 크기를 바꿔도 빠뜨림 없음")
{
    CheckNoMissAgainstBruteForce(32.0f);
    CheckNoMissAgainstBruteForce(512.0f);
}
