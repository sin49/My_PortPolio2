#include "audio/Audio_C.h"

#include <vector>

Audio::Audio()
{
    std::vector<std::int16_t> vecSamples;  // 합성 버퍼 재사용 (SoundAsset이 복사해 감)
    for (std::size_t i = 0; i < kSfxCount; ++i)
    {
        SynthesizeSfx(GetSfxRecipe(static_cast<SfxId>(i)), vecSamples);
        m_arrSounds[i] = SoundAsset(vecSamples, kSfxSampleRate);  // 이동 대입 (임시 객체 -> 배열 칸)
    }
}

void Audio::PlayQueued()
{
    std::uniform_real_distribution<float> tPitch(0.94f, 1.06f);
    for (std::size_t i = 0; i < kSfxCount; ++i)
    {
        if (!m_arrQueued[i])
            continue;
        m_arrQueued[i] = false;
        m_arrSounds[i].Play(1.0f, tPitch(m_tRng));
    }
}
