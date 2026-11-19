#include "audio/SfxSynth_C.h"

#include <algorithm>
#include <cmath>

namespace
{
// 순서 = SfxId 순서 (아래 static_assert)
constexpr std::array<SfxRecipe, kSfxCount> kRecipes = {{
    // Swing: 짧은 "쉭"
    {.eWave = Waveform::Noise, .fDuration = 0.10f, .fNoiseMix = 1.0f, .fNoiseSmooth = 0.35f, .fVolume = 0.30f},
    // Hit: 내려가는 사각파 + 잡음 약간
    {.eWave = Waveform::Square, .fDuration = 0.09f, .fFreqStart = 260.0f, .fFreqEnd = 90.0f, .fNoiseMix = 0.3f,
     .fVolume = 0.35f},
    // Kill: 더 길고 낮게
    {.eWave = Waveform::Square, .fDuration = 0.22f, .fFreqStart = 360.0f, .fFreqEnd = 50.0f, .fNoiseMix = 0.45f,
     .fNoiseSmooth = 0.3f, .fVolume = 0.40f},
    // Hurt: 낮고 거친 소리
    {.eWave = Waveform::Square, .fDuration = 0.25f, .fFreqStart = 170.0f, .fFreqEnd = 60.0f, .fNoiseMix = 0.35f,
     .fVolume = 0.45f},
    // Shoot: 짧은 "삐융"
    {.eWave = Waveform::Square, .fDuration = 0.08f, .fFreqStart = 950.0f, .fFreqEnd = 420.0f, .fVolume = 0.18f},
    // Pickup: 두 음 올라감
    {.eWave = Waveform::Triangle, .fDuration = 0.22f, .fFreqStart = 660.0f, .fFreqEnd = 660.0f, .fVolume = 0.45f,
     .arrNoteRatios = {1.0f, 1.5f, 1.0f, 1.0f}, .iNoteCount = 2},
    // Dash: 둔탁한 "휙"
    {.eWave = Waveform::Noise, .fDuration = 0.16f, .fNoiseMix = 1.0f, .fNoiseSmooth = 0.15f, .fVolume = 0.35f},
    // RoomClear: 도-미-솔-도
    {.eWave = Waveform::Triangle, .fDuration = 0.48f, .fFreqStart = 523.0f, .fFreqEnd = 523.0f, .fVolume = 0.45f,
     .arrNoteRatios = {1.0f, 1.26f, 1.5f, 2.0f}, .iNoteCount = 4},
}};
static_assert(kRecipes.size() == kSfxCount, "SfxId마다 레시피 1개");
} // namespace

const SfxRecipe& GetSfxRecipe(SfxId eSfx)
{
    return kRecipes[static_cast<std::size_t>(eSfx)];
}

void SynthesizeSfx(const SfxRecipe& tRecipe, std::vector<std::int16_t>& vecOut)
{
    vecOut.clear();
    const int iSamples = std::max(1, static_cast<int>(tRecipe.fDuration * static_cast<float>(kSfxSampleRate)));
    vecOut.reserve(static_cast<std::size_t>(iSamples));

    std::uint32_t uNoiseState = 0x1234567u;  // 고정 시드 -> 항상 같은 소리
    float fNoiseLow = 0.0f;
    float fPhase = 0.0f;  // 0 ~ 1
    const int iNotes = std::clamp(tRecipe.iNoteCount, 1, static_cast<int>(tRecipe.arrNoteRatios.size()));

    for (int i = 0; i < iSamples; ++i)
    {
        const float fT = static_cast<float>(i) / static_cast<float>(iSamples);  // 0 ~ 1
        const int iNote = std::min(iNotes - 1, static_cast<int>(fT * static_cast<float>(iNotes)));
        const float fFreq = tRecipe.fFreqStart * std::pow(tRecipe.fFreqEnd / tRecipe.fFreqStart, fT) *
                            tRecipe.arrNoteRatios[static_cast<std::size_t>(iNote)];
        fPhase += fFreq / static_cast<float>(kSfxSampleRate);
        fPhase -= std::floor(fPhase);

        float fTone = 0.0f;
        if (tRecipe.eWave == Waveform::Square)
            fTone = fPhase < 0.5f ? 1.0f : -1.0f;
        else if (tRecipe.eWave == Waveform::Triangle)
            fTone = 1.0f - 4.0f * std::abs(fPhase - 0.5f);

        // xorshift 잡음 -> 저역 통과 (부드럽게)
        uNoiseState ^= uNoiseState << 13;
        uNoiseState ^= uNoiseState >> 17;
        uNoiseState ^= uNoiseState << 5;
        const float fWhite = static_cast<float>(uNoiseState & 0xFFFFu) / 32767.5f - 1.0f;
        fNoiseLow += tRecipe.fNoiseSmooth * (fWhite - fNoiseLow);

        const float fMix = tRecipe.eWave == Waveform::Noise ? 1.0f : tRecipe.fNoiseMix;
        const float fRaw = fTone * (1.0f - fMix) + fNoiseLow * fMix;

        // 음 하나마다: 짧게 올라갔다가 (딸깍 방지) 2제곱으로 줄어듦
        const float fNoteT = fT * static_cast<float>(iNotes) - static_cast<float>(iNote);
        const float fAttack = std::min(1.0f, fNoteT * 40.0f);
        const float fDecay = (1.0f - fNoteT) * (1.0f - fNoteT);
        const float fSample = std::clamp(fRaw * fAttack * fDecay * tRecipe.fVolume, -1.0f, 1.0f);
        vecOut.push_back(static_cast<std::int16_t>(fSample * 32767.0f));
    }
}
