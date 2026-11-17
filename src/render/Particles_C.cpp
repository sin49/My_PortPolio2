#include "render/Particles_C.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{
// 튜닝 값
constexpr SparkShape kHitSparks = {.iCountMin = 10, .iCountMax = 16, .fSpreadDeg = 50.0f, .fSpeedMin = 160.0f,
                                   .fSpeedMax = 500.0f, .fLifeMin = 0.20f, .fLifeMax = 0.50f, .fSize = 6.0f,
                                   .fScatterRatio = 0.3f, .fSizeJitter = 0.5f};
constexpr SparkShape kDeathBurst = {.iCountMin = 28, .iCountMax = 40, .fSpreadDeg = 80.0f, .fSpeedMin = 120.0f,
                                    .fSpeedMax = 600.0f, .fLifeMin = 0.30f, .fLifeMax = 0.80f, .fSize = 8.0f,
                                    .fScatterRatio = 0.4f, .fSizeJitter = 0.5f};
constexpr float kGravity = 900.0f;  // 게임 중력보다 약하게
} // namespace

void Particles::SpawnHitSparks(Vec2 vPos, int iDir, bool bKilled, float fScale)
{
    Spawn(vPos, iDir, kHitSparks, fScale);
    if (bKilled)
        Spawn(vPos, iDir, kDeathBurst, fScale);  // 죽을 때는 크게 한 번 더
}

// fScale: 맞은 대상 크기. 클수록 파편이 많고 크고 조금 더 멀리 (작은 적 = 1)
void Particles::Spawn(Vec2 vPos, int iDir, const SparkShape& tShape, float fScale)
{
    std::uniform_int_distribution<int> tCountDist(tShape.iCountMin, tShape.iCountMax);
    std::uniform_real_distribution<float> tAngleDist(-tShape.fSpreadDeg, tShape.fSpreadDeg);
    std::uniform_real_distribution<float> tSpeedDist(tShape.fSpeedMin, tShape.fSpeedMax);
    std::uniform_real_distribution<float> tLifeDist(tShape.fLifeMin, tShape.fLifeMax);
    std::uniform_real_distribution<float> tUnitDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> tFullCircleDist(-180.0f, 180.0f);
    std::uniform_real_distribution<float> tSizeDist(1.0f - tShape.fSizeJitter, 1.0f + tShape.fSizeJitter);

    const float fSizeScale = std::pow(fScale, 0.7f);          // 큰 적 1.79 -> 파편 크기 약 1.5배
    const float fSpeedScale = 0.85f + 0.15f * fScale;          // 큰 적 -> 약 1.12배 멀리
    int iRemaining = static_cast<int>(static_cast<float>(tCountDist(m_tRng)) * fScale);  // 큰 적 -> 개수 약 1.8배
    for (Particle& tParticle : m_arrParticles)
    {
        if (iRemaining <= 0)
            break;
        if (tParticle.fLife > 0.0f)
            continue;

        // 일부(fScatterRatio)는 사방으로, 나머지는 iDir 쪽 부채꼴로
        const bool bScatter = tUnitDist(m_tRng) < tShape.fScatterRatio;
        const float fAngleDeg = bScatter ? tFullCircleDist(m_tRng) : tAngleDist(m_tRng);
        const float fAngle = fAngleDeg * std::numbers::pi_v<float> / 180.0f;
        const float fDirX = bScatter ? 1.0f : static_cast<float>(iDir);  // 사방이면 각도가 이미 360도
        const float fSpeed = tSpeedDist(m_tRng) * (bScatter ? 0.6f : 1.0f) * fSpeedScale;  // 사방 파편은 조금 느리게
        tParticle.vPos = vPos;
        tParticle.vVel = {fDirX * std::cos(fAngle) * fSpeed, std::sin(fAngle) * fSpeed};
        tParticle.fMaxLife = tLifeDist(m_tRng);
        tParticle.fLife = tParticle.fMaxLife;
        tParticle.fSize = tShape.fSize * tSizeDist(m_tRng) * fSizeScale;
        --iRemaining;
    }
}

void Particles::SpawnGhost(const Rect& rcBody)
{
    // 빈 칸이 없으면 가장 오래된(수명이 제일 적은) 잔상을 덮어씀
    Ghost* pSlot = &m_arrGhosts[0];
    for (Ghost& tGhost : m_arrGhosts)
    {
        if (tGhost.fLife <= 0.0f)
        {
            pSlot = &tGhost;
            break;
        }
        if (tGhost.fLife < pSlot->fLife)
            pSlot = &tGhost;
    }
    pSlot->rcBody = rcBody;
    pSlot->fLife = kGhostLife;
}

void Particles::Update(float fDt)
{
    for (Ghost& tGhost : m_arrGhosts)
        tGhost.fLife = std::max(0.0f, tGhost.fLife - fDt);

    for (Particle& tParticle : m_arrParticles)
    {
        if (tParticle.fLife <= 0.0f)
            continue;
        tParticle.vVel.fY += kGravity * fDt;
        tParticle.vPos.fX += tParticle.vVel.fX * fDt;
        tParticle.vPos.fY += tParticle.vVel.fY * fDt;
        tParticle.fLife -= fDt;
    }
}
