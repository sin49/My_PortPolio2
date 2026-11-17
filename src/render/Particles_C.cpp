#include "render/Particles_C.h"

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

void Particles::SpawnHitSparks(Vec2 vPos, int iDir, bool bKilled)
{
    Spawn(vPos, iDir, kHitSparks);
    if (bKilled)
        Spawn(vPos, iDir, kDeathBurst);  // 죽을 때는 크게 한 번 더
}

void Particles::Spawn(Vec2 vPos, int iDir, const SparkShape& tShape)
{
    std::uniform_int_distribution<int> tCountDist(tShape.iCountMin, tShape.iCountMax);
    std::uniform_real_distribution<float> tAngleDist(-tShape.fSpreadDeg, tShape.fSpreadDeg);
    std::uniform_real_distribution<float> tSpeedDist(tShape.fSpeedMin, tShape.fSpeedMax);
    std::uniform_real_distribution<float> tLifeDist(tShape.fLifeMin, tShape.fLifeMax);
    std::uniform_real_distribution<float> tUnitDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> tFullCircleDist(-180.0f, 180.0f);
    std::uniform_real_distribution<float> tSizeDist(1.0f - tShape.fSizeJitter, 1.0f + tShape.fSizeJitter);

    int iRemaining = tCountDist(m_tRng);
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
        const float fSpeed = tSpeedDist(m_tRng) * (bScatter ? 0.6f : 1.0f);  // 사방 파편은 조금 느리게
        tParticle.vPos = vPos;
        tParticle.vVel = {fDirX * std::cos(fAngle) * fSpeed, std::sin(fAngle) * fSpeed};
        tParticle.fMaxLife = tLifeDist(m_tRng);
        tParticle.fLife = tParticle.fMaxLife;
        tParticle.fSize = tShape.fSize * tSizeDist(m_tRng);
        --iRemaining;
    }
}

void Particles::Update(float fDt)
{
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
