#pragma once

#include "core/Vec2_C.h"

#include <array>
#include <random>

// 연출 전용 파편 (게임 판정에 영향 없음 -> World에 넣지 않음). 시뮬레이션만, 그리기는 Render.
struct Particle
{
    Vec2 vPos = {};
    Vec2 vVel = {};
    float fLife = 0.0f;     // <= 0 이면 빈 칸
    float fMaxLife = 0.0f;
    float fSize = 0.0f;     // 시작 크기(px)
};

// 파편 한 묶음의 모양 (때릴 때 / 죽을 때 값은 Particles_C.cpp)
struct SparkShape
{
    int iCountMin = 0;
    int iCountMax = 0;
    float fSpreadDeg = 0.0f;  // iDir 방향 기준 위아래 퍼짐
    float fSpeedMin = 0.0f;
    float fSpeedMax = 0.0f;
    float fLifeMin = 0.0f;
    float fLifeMax = 0.0f;
    float fSize = 0.0f;       // 시작 크기(px). 수명 끝에서 1/3로 줄어듦
    float fScatterRatio = 0.0f;  // 이 비율만큼은 방향 무시하고 사방으로 (0~1)
    float fSizeJitter = 0.0f;    // 크기 랜덤 폭 (0.4 = ±40%)
};

class Particles
{
public:
    static constexpr int kCapacity = 256;  // 다 차면 새 파편은 무시

    // 맞은 지점에서 iDir 쪽(맞은 쪽 뒤)으로 하얀 파편. bKilled면 크게 한 번 더
    void SpawnHitSparks(Vec2 vPos, int iDir, bool bKilled);
    void Update(float fDt);

    const std::array<Particle, kCapacity>& GetAll() const { return m_arrParticles; }

private:
    void Spawn(Vec2 vPos, int iDir, const SparkShape& tShape);

    std::array<Particle, kCapacity> m_arrParticles = {};
    std::mt19937 m_tRng{20261118u};  // 고정 시드 -> 같은 입력이면 같은 모양 (전역 아님)
};
