#pragma once
// raylib Sound 1개의 수명을 타입으로 관리 (RAII). docs/design/stress-sound.md 2장, user-tasks.md 3장
// - 생성자에서 Load, 소멸자에서 Unload -> 해제 잊음(누수) 없음
// - 복사 금지 -> 두 객체가 같은 자원을 두 번 Unload(이중 해제) 못 함
// - 이동 허용 -> 자원을 넘겨받고 원래 쪽은 "빈 상태"(소멸자에서 아무것도 안 함)
#include "raylib.h"

#include <cstdint>
#include <span>

class SoundAsset
{
public:
    SoundAsset() = default;  // 빈 상태 (오디오 장치가 없을 때 등)
    // 16비트 모노 샘플에서 생성. raylib이 내부 버퍼로 복사하므로 spSamples는 호출 뒤 버려도 됨
    SoundAsset(std::span<const std::int16_t> spSamples, int iSampleRate);
    ~SoundAsset();

    SoundAsset(const SoundAsset&) = delete;
    SoundAsset& operator=(const SoundAsset&) = delete;
    SoundAsset(SoundAsset&& tOther) noexcept;
    SoundAsset& operator=(SoundAsset&& tOther) noexcept;

    bool IsLoaded() const { return m_bLoaded; }
    void Play(float fVolume = 1.0f, float fPitch = 1.0f);  // 빈 상태면 아무것도 안 함

private:
    void Release();  // 들고 있으면 Unload 후 빈 상태로

    Sound m_tSound = {};
    bool m_bLoaded = false;
};

// 오디오 장치 열기·닫기도 같은 방식 (main에서 Game보다 먼저 만들어 Game보다 늦게 파괴)
class AudioDeviceScope
{
public:
    AudioDeviceScope();
    ~AudioDeviceScope();
    AudioDeviceScope(const AudioDeviceScope&) = delete;
    AudioDeviceScope& operator=(const AudioDeviceScope&) = delete;
};
