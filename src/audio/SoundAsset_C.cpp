#include "audio/SoundAsset_C.h"

#include <type_traits>
#include <utility>

// 규칙을 컴파일 시간에 검사: 복사되면 이중 해제, 이동이 throw하면 vector 재배치 때 복사로 떨어짐
static_assert(!std::is_copy_constructible_v<SoundAsset>);
static_assert(!std::is_copy_assignable_v<SoundAsset>);
static_assert(std::is_nothrow_move_constructible_v<SoundAsset>);
static_assert(std::is_nothrow_move_assignable_v<SoundAsset>);
static_assert(!std::is_copy_constructible_v<AudioDeviceScope>);

SoundAsset::SoundAsset(std::span<const std::int16_t> spSamples, int iSampleRate)
{
    if (!IsAudioDeviceReady() || spSamples.empty())
        return;  // 장치가 없으면 소리 없이 진행 (게임은 계속)
    Wave tWave = {};
    tWave.frameCount = static_cast<unsigned int>(spSamples.size());
    tWave.sampleRate = static_cast<unsigned int>(iSampleRate);
    tWave.sampleSize = 16;
    tWave.channels = 1;
    // raylib Wave.data는 void* (C API라 const가 없음). LoadSoundFromWave는 읽기만 하고 복사함
    tWave.data = const_cast<std::int16_t*>(spSamples.data());
    m_tSound = LoadSoundFromWave(tWave);
    m_bLoaded = IsSoundValid(m_tSound);
}

SoundAsset::~SoundAsset()
{
    Release();
}

SoundAsset::SoundAsset(SoundAsset&& tOther) noexcept
    : m_tSound(std::exchange(tOther.m_tSound, Sound{}))
    , m_bLoaded(std::exchange(tOther.m_bLoaded, false))
{
}

SoundAsset& SoundAsset::operator=(SoundAsset&& tOther) noexcept
{
    if (this != &tOther)
    {
        Release();  // 내 자원 먼저 해제 -> 넘겨받음
        m_tSound = std::exchange(tOther.m_tSound, Sound{});
        m_bLoaded = std::exchange(tOther.m_bLoaded, false);
    }
    return *this;
}

void SoundAsset::Play(float fVolume, float fPitch)
{
    if (!m_bLoaded)
        return;
    SetSoundVolume(m_tSound, fVolume);
    SetSoundPitch(m_tSound, fPitch);
    PlaySound(m_tSound);
}

void SoundAsset::Release()
{
    if (!m_bLoaded)
        return;
    UnloadSound(m_tSound);
    m_tSound = Sound{};
    m_bLoaded = false;
}

AudioDeviceScope::AudioDeviceScope()
{
    InitAudioDevice();
}

AudioDeviceScope::~AudioDeviceScope()
{
    if (IsAudioDeviceReady())
        CloseAudioDevice();
}
