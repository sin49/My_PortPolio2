#pragma once
// 효과음 재생 (연출 전용, 판정 무관). Game이 틱 결과(HitReport 등)를 보고 Queue -> 프레임 끝에 PlayQueued
// 같은 소리를 한 프레임에 여러 번 요청해도 1번만 재생 (스트레스 소환 200마리가 동시에 맞아도 귀가 안전)
#include "audio/SfxSynth_C.h"
#include "audio/SoundAsset_C.h"

#include <array>
#include <random>

class Audio
{
public:
    Audio();  // 오디오 장치가 열린 뒤에 만들어야 함 (main의 AudioDeviceScope)

    void Queue(SfxId eSfx) { m_arrQueued[static_cast<std::size_t>(eSfx)] = true; }
    void PlayQueued();  // 요청된 것만 재생하고 요청 비움. 피치를 살짝 흔들어 반복이 덜 거슬리게

private:
    std::array<SoundAsset, kSfxCount> m_arrSounds;
    std::array<bool, kSfxCount> m_arrQueued = {};
    std::mt19937 m_tRng{20261119u};
};
