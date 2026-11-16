#include "core/FixedTimestep_C.h"

#include <doctest/doctest.h>

TEST_CASE("FixedTimestep: 반 스텝씩 두 번 들어오면 두 번째에 1회 업데이트")
{
    FixedTimestep tTimestep;
    CHECK(tTimestep.Advance(FixedTimestep::kStep * 0.5f) == 0);
    CHECK(tTimestep.Advance(FixedTimestep::kStep * 0.5f) == 1);
}

TEST_CASE("FixedTimestep: 긴 프레임은 kMaxFrameTime으로 잘림")
{
    FixedTimestep tClamped;
    FixedTimestep tReference;
    CHECK(tClamped.Advance(10.0f) == tReference.Advance(FixedTimestep::kMaxFrameTime));
}

TEST_CASE("FixedTimestep: 음수 프레임 시간은 무시")
{
    FixedTimestep tTimestep;
    CHECK(tTimestep.Advance(-1.0f) == 0);
    CHECK(tTimestep.GetAlpha() == doctest::Approx(0.0f));
}
