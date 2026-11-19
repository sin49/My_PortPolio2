#include "systems/Projectile_C.h"

#include "core/EntitySide_C.h"
#include "core/Tilemap_C.h"
#include "core/World_U.h"
#include "data/ProjectileDef_C.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{
Vec2 Normalize(Vec2 vDir)
{
    const float fLen = std::sqrt(vDir.fX * vDir.fX + vDir.fY * vDir.fY);
    return fLen > 0.0f ? Vec2{vDir.fX / fLen, vDir.fY / fLen} : Vec2{};
}

Vec2 Rotate(Vec2 vDir, float fDeg)
{
    const float fRad = fDeg * std::numbers::pi_v<float> / 180.0f;
    const float fCos = std::cos(fRad);
    const float fSin = std::sin(fRad);
    return Vec2{vDir.fX * fCos - vDir.fY * fSin, vDir.fX * fSin + vDir.fY * fCos};
}

Vec2 CenterOf(const Body& tBody)
{
    return Vec2{tBody.vPos.fX + tBody.vSize.fX * 0.5f, tBody.vPos.fY + tBody.vSize.fY * 0.5f};
}
} // namespace

Handle Projectiles::Spawn(World& tWorld, Vec2 vCenter, Vec2 vDir, const ProjectileDef& tDef, Faction eOwner) const
{
    if (tWorld.GetAliveCount() >= static_cast<int>(World::kCapacity))
        return Handle{};  // 풀이 꽉 참 (스트레스 테스트) -> 이번 탄은 안 만듦. Create의 throw는 치명적 오류용이라 피함
    const Vec2 vUnit = Normalize(vDir);
    Entity tBullet;
    tBullet.eType = EntityType::Projectile;
    tBullet.tBody = {.vPos = {vCenter.fX - tDef.vSize.fX * 0.5f, vCenter.fY - tDef.vSize.fY * 0.5f},
                     .vSize = tDef.vSize,
                     .vVel = {vUnit.fX * tDef.fSpeed, vUnit.fY * tDef.fSpeed}};
    tBullet.fHp = 1.0f;  // 몸 HP 아님. "살아 있음" 표시 (fHp <= 0 = 삭제 예약으로 보는 코드가 있어서)
    tBullet.tProjectile = {.fLife = tDef.fLifetime, .fDamage = tDef.fDamage, .eOwner = eOwner};
    return tWorld.Create(tBullet);
}

int Projectiles::FireFromIntents(World& tWorld, const ProjectileDef& tDef)
{
    // 1) 모으기: 순회 중에 Create하면 같은 순회에서 새 탄을 또 방문할 수 있음 -> 순회 밖에서 생성
    m_vecRequests.clear();
    tWorld.ForEachAlive([&](Handle, Entity& tEntity) {
        AttackIntent& tIntent = tEntity.tAttackIntent;
        if (!tIntent.bFirePressed || tEntity.fHp <= 0.0f)
            return;
        tIntent.bFirePressed = false;  // 소비
        if (tIntent.vAim.fX == 0.0f && tIntent.vAim.fY == 0.0f)
            return;
        m_vecRequests.push_back({.vCenter = CenterOf(tEntity.tBody),
                                 .vAim = tIntent.vAim,
                                 .iShotCount = tIntent.iShotCount,
                                 .fSpreadDeg = tIntent.fSpreadDeg,
                                 .eOwner = GetFaction(tEntity.eType)});
    });

    // 2) 생성: 부채꼴은 가운데 기준 좌우로 고르게, 360 이상은 원형으로 고르게
    for (const FireRequest& tRequest : m_vecRequests)
    {
        const int iCount = std::max(1, tRequest.iShotCount);
        for (int i = 0; i < iCount; ++i)
        {
            float fOffsetDeg = 0.0f;
            if (tRequest.fSpreadDeg >= 360.0f)
                fOffsetDeg = 360.0f * static_cast<float>(i) / static_cast<float>(iCount);
            else if (iCount > 1)
                fOffsetDeg = -tRequest.fSpreadDeg * 0.5f +
                             tRequest.fSpreadDeg * static_cast<float>(i) / static_cast<float>(iCount - 1);
            Spawn(tWorld, tRequest.vCenter, Rotate(tRequest.vAim, fOffsetDeg), tDef, tRequest.eOwner);
        }
    }
    return static_cast<int>(m_vecRequests.size());
}

void Projectiles::Update(World& tWorld, const Tilemap& tMap, float fDt, std::vector<Vec2>& vecOutWallHits) const
{
    tWorld.ForEachAlive([&](Handle hBullet, Entity& tEntity) {
        if (tEntity.eType != EntityType::Projectile || tEntity.tProjectile.fLife <= 0.0f)
            return;
        Body& tBody = tEntity.tBody;
        tBody.vPos.fX += tBody.vVel.fX * fDt;
        tBody.vPos.fY += tBody.vVel.fY * fDt;
        tEntity.tProjectile.fLife -= fDt;

        const Vec2 vCenter = CenterOf(tBody);
        const bool bHitWall = tMap.IsSolid(static_cast<int>(std::floor(vCenter.fX / kTileSize)),
                                           static_cast<int>(std::floor(vCenter.fY / kTileSize)));
        if (bHitWall)
            vecOutWallHits.push_back(vCenter);
        if (bHitWall || tEntity.tProjectile.fLife <= 0.0f)
        {
            tEntity.tProjectile.fLife = 0.0f;  // 같은 틱 판정에서 제외
            tWorld.Destroy(hBullet);           // 예약만 (Flush는 틱 끝)
        }
    });
}
