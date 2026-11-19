#include "systems/LedgeGuard_C.h"

#include "core/EntitySide_C.h"
#include "core/Tilemap_C.h"
#include "core/World_U.h"

#include <cmath>

namespace
{
// 몸이 iDir 쪽으로 kLookAhead만큼 가면 발밑(몸 바로 아래 줄)이 비었나
bool IsLedgeAhead(const Body& tBody, int iDir, const Tilemap& tRoom)
{
    const float fProbeX = iDir > 0 ? tBody.vPos.fX + tBody.vSize.fX + LedgeGuard::kLookAhead
                                   : tBody.vPos.fX - LedgeGuard::kLookAhead;
    const float fProbeY = tBody.vPos.fY + tBody.vSize.fY + 1.0f;  // 발 바로 아래
    const int iTileX = static_cast<int>(std::floor(fProbeX / kTileSize));
    const int iTileY = static_cast<int>(std::floor(fProbeY / kTileSize));
    return !tRoom.IsSolid(iTileX, iTileY);
}
} // namespace

void LedgeGuard::Apply(World& tWorld, const Tilemap& tRoom) const
{
    tWorld.ForEachAlive([&](Handle, Entity& tEntity) {
        if (!IsEnemySide(tEntity.eType) || !tEntity.tBody.bGrounded)
            return;  // 공중(넉백으로 밀려남 등)은 그대로
        const int iDir = tEntity.tMoveIntent.iMoveDir;
        if (iDir != 0 && IsLedgeAhead(tEntity.tBody, iDir, tRoom))
            tEntity.tMoveIntent.iMoveDir = 0;
    });
}
