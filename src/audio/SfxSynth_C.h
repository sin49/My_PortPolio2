#pragma once
// 효과음 파형 합성 (docs/design/stress-sound.md 2장). 파일·다운로드 없이 코드로 만든 16비트 모노 샘플.
// raylib을 모름 -> GameLogic에 들어가 테스트 가능. raylib Sound로 바꾸는 건 SoundAsset_C (Main 쪽)
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

enum class SfxId
{
    Swing,      // 플레이어 휘두름
    Hit,        // 적이 맞음
    Kill,       // 적 처치
    Hurt,       // 플레이어가 맞음
    Shoot,      // 적 발사
    Pickup,     // 아이템 줍기
    Dash,       // 대시 시작
    RoomClear,  // 방 클리어 (문 열림)
    Count,
};

inline constexpr std::size_t kSfxCount = static_cast<std::size_t>(SfxId::Count);
inline constexpr int kSfxSampleRate = 22050;

enum class Waveform
{
    Square,
    Triangle,
    Noise,  // 잡음만 (휘두름·대시의 "쉭")
};

// 한 효과음 = 파형 + 길이 + 주파수 미끄러짐 + (선택) 음 계단 + 잡음 섞기
struct SfxRecipe
{
    Waveform eWave = Waveform::Square;
    float fDuration = 0.1f;   // 초
    float fFreqStart = 440.0f;
    float fFreqEnd = 440.0f;  // 시작 -> 끝으로 지수 보간 (내려가면 "툭", 올라가면 "삥")
    float fNoiseMix = 0.0f;   // 0 ~ 1, 잡음 비율
    float fNoiseSmooth = 0.5f;  // 잡음 저역 통과 (작을수록 둔탁)
    float fVolume = 0.5f;
    std::array<float, 4> arrNoteRatios = {1.0f, 1.0f, 1.0f, 1.0f};  // 길이를 iNoteCount로 나눠 음높이 배율
    int iNoteCount = 1;
};

const SfxRecipe& GetSfxRecipe(SfxId eSfx);

// vecOut을 비우고 샘플을 채움 (길이 = fDuration * kSfxSampleRate). 같은 입력이면 항상 같은 결과
void SynthesizeSfx(const SfxRecipe& tRecipe, std::vector<std::int16_t>& vecOut);
