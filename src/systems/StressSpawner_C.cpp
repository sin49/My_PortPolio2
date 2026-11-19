#include "systems/StressSpawner_C.h"

#include "core/Tilemap_C.h"
#include "core/World_U.h"
#include "data/EntityFactory_C.h"

#include <cmath>

namespace
{
// 사각형이 걸치는 타일 중 하나라도 벽이면 true (방 밖도 벽 취급)
bool TouchesSolid(const Tilemap& tRoom, const Rect& rcArea)
{
    const int iMinX = static_cast<int>(std::floor(rcArea.fX / kTileSize));
    const int iMaxX = static_cast<int>(std::floor((rcArea.fX + rcArea.fW) / kTileSize));
    const int iMinY = static_cast<int>(std::floor(rcArea.fY / kTileSize));
    const int iMaxY = static_cast<int>(std::floor((rcArea.fY + rcArea.fH) / kTileSize));
    for (int iY = iMinY; iY <= iMaxY; ++iY)
        for (int iX = iMinX; iX <= iMaxX; ++iX)
            if (tRoom.IsSolid(iX, iY))
                return true;
    return false;
}
} // namespace

StressSpawner::StressSpawner(std::uint32_t uSeed)
    : m_tRng(uSeed)
{
}

bool StressSpawner::SpawnSmallChaser(World& tWorld, const Tilemap& tRoom, Handle hPlayer, const StressSpawnDef& tDef)
{
    const int iFree = static_cast<int>(World::kCapacity) - tWorld.GetAliveCount();
    if (iFree <= tDef.iReservedSlots)
        return false;

    const Vec2 vSize = GetEnemySizeDef(EnemySize::Small).vSize;
    // 바깥 테두리 1칸 안쪽 어디든 (공중이면 중력으로 떨어짐)
    std::uniform_real_distribution<float> tRandX(kTileSize, static_cast<float>(tRoom.GetCols() - 1) * kTileSize - vSize.fX);
    std::uniform_real_distribution<float> tRandY(kTileSize, static_cast<float>(tRoom.GetRows() - 1) * kTileSize - vSize.fY);

    const Entity* pPlayer = tWorld.Get(hPlayer);
    for (int iTry = 0; iTry < tDef.iMaxTries; ++iTry)
    {
        const Rect rcBody = {tRandX(m_tRng), tRandY(m_tRng), vSize.fX, vSize.fY};
        if (TouchesSolid(tRoom, rcBody))
            continue;
        if (pPlayer != nullptr)
        {
            const Body& tPlayerBody = pPlayer->tBody;
            const float fDx = (rcBody.fX + rcBody.fW * 0.5f) - (tPlayerBody.vPos.fX + tPlayerBody.vSize.fX * 0.5f);
            const float fDy = (rcBody.fY + rcBody.fH * 0.5f) - (tPlayerBody.vPos.fY + tPlayerBody.vSize.fY * 0.5f);
            if (fDx * fDx + fDy * fDy < tDef.fMinPlayerDistance * tDef.fMinPlayerDistance)
                continue;
        }
        const EnemySpawn tSpawn = {.eKind = SpawnKind::Chaser, .eSize = EnemySize::Small};
        tWorld.Create(MakeEnemyEntity(tSpawn, {rcBody.fX + rcBody.fW * 0.5f, rcBody.fY + rcBody.fH}));
        return true;
    }
    return false;
}
