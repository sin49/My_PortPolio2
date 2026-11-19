// 효과음 합성 테스트 (docs/design/stress-sound.md 2장). raylib 없이 샘플만 검사
#include "audio/SfxSynth_C.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstdlib>

TEST_CASE("SfxSynth: 모든 효과음이 길이대로 생성되고 무음이 아니며 끝은 조용함 (딸깍 없음)")
{
    std::vector<std::int16_t> vecSamples;
    for (std::size_t i = 0; i < kSfxCount; ++i)
    {
        const SfxRecipe& tRecipe = GetSfxRecipe(static_cast<SfxId>(i));
        SynthesizeSfx(tRecipe, vecSamples);
        CAPTURE(i);
        CHECK(vecSamples.size() == static_cast<std::size_t>(tRecipe.fDuration * static_cast<float>(kSfxSampleRate)));
        int iPeak = 0;
        for (const std::int16_t iSample : vecSamples)
            iPeak = std::max(iPeak, std::abs(static_cast<int>(iSample)));
        CHECK(iPeak > 1000);
        CHECK(std::abs(static_cast<int>(vecSamples.front())) < 2000);  // 시작 = 짧게 올라옴
        CHECK(std::abs(static_cast<int>(vecSamples.back())) < 500);    // 끝 = 줄어듦
    }
}

TEST_CASE("SfxSynth: 같은 레시피면 항상 같은 샘플 (버퍼 재사용해도)")
{
    std::vector<std::int16_t> vecA;
    std::vector<std::int16_t> vecB;
    SynthesizeSfx(GetSfxRecipe(SfxId::Kill), vecA);
    SynthesizeSfx(GetSfxRecipe(SfxId::Pickup), vecB);
    SynthesizeSfx(GetSfxRecipe(SfxId::Kill), vecB);
    CHECK(vecA == vecB);
}
