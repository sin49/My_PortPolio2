#include "systems/Physics_C.h"

#include "core/Body_C.h"
#include "core/FixedTimestep_C.h"
#include "core/Tilemap_C.h"
#include "data/PhysicsDef_C.h"

#include <algorithm>
#include <cmath>

static_assert(kPhysicsDef.fMaxFallSpeed * FixedTimestep::kStep < kTileSize,
              "한 틱에 타일 하나보다 많이 떨어지면 바닥을 뚫을 수 있음");

namespace
{
// 오른쪽/아래 경계를 타일로 바꿀 때 빼는 값.
// 타일 경계에 딱 붙어 있을 때 옆 타일까지 겹친다고 오판하지 않게 함.
constexpr float kEdgeEpsilon = 0.001f;

int ToTile(float fPx)
{
    return static_cast<int>(std::floor(fPx / kTileSize));
}

float ToPixel(int iTile)
{
    return static_cast<float>(iTile) * kTileSize;
}

bool AnySolidInColumn(const Tilemap& tMap, int iTileX, int iTileYMin, int iTileYMax)
{
    for (int iTileY = iTileYMin; iTileY <= iTileYMax; ++iTileY)
        if (tMap.IsSolid(iTileX, iTileY)) return true;
    return false;
}

bool AnySolidInRow(const Tilemap& tMap, int iTileY, int iTileXMin, int iTileXMax)
{
    for (int iTileX = iTileXMin; iTileX <= iTileXMax; ++iTileX)
        if (tMap.IsSolid(iTileX, iTileY)) return true;
    return false;
}

// 한 틱 이동량 < 타일 크기라서 진행 방향의 맨 끝 열/행만 보면 충분함.
void ResolveX(Body& tBody, const Tilemap& tMap)
{
    if (tBody.vVel.fX == 0.0f) return;

    const int iTop = ToTile(tBody.vPos.fY);
    const int iBottom = ToTile(tBody.vPos.fY + tBody.vSize.fY - kEdgeEpsilon);

    if (tBody.vVel.fX > 0.0f)
    {
        const int iRight = ToTile(tBody.vPos.fX + tBody.vSize.fX - kEdgeEpsilon);
        if (AnySolidInColumn(tMap, iRight, iTop, iBottom))
        {
            tBody.vPos.fX = ToPixel(iRight) - tBody.vSize.fX;
            tBody.vVel.fX = 0.0f;
        }
    }
    else
    {
        const int iLeft = ToTile(tBody.vPos.fX);
        if (AnySolidInColumn(tMap, iLeft, iTop, iBottom))
        {
            tBody.vPos.fX = ToPixel(iLeft + 1);
            tBody.vVel.fX = 0.0f;
        }
    }
}

void ResolveY(Body& tBody, const Tilemap& tMap)
{
    if (tBody.vVel.fY == 0.0f) return;

    const int iLeft = ToTile(tBody.vPos.fX);
    const int iRight = ToTile(tBody.vPos.fX + tBody.vSize.fX - kEdgeEpsilon);

    if (tBody.vVel.fY > 0.0f)
    {
        const int iBottom = ToTile(tBody.vPos.fY + tBody.vSize.fY - kEdgeEpsilon);
        if (AnySolidInRow(tMap, iBottom, iLeft, iRight))
        {
            tBody.vPos.fY = ToPixel(iBottom) - tBody.vSize.fY;
            tBody.vVel.fY = 0.0f;
            tBody.bGrounded = true;
        }
    }
    else
    {
        const int iTop = ToTile(tBody.vPos.fY);
        if (AnySolidInRow(tMap, iTop, iLeft, iRight))
        {
            tBody.vPos.fY = ToPixel(iTop + 1);
            tBody.vVel.fY = 0.0f;
        }
    }
}
} // namespace

void Physics::Update(Body& tBody, const Tilemap& tMap, float fDt) const
{
    if (!tBody.bNoGravity)
        tBody.vVel.fY = std::min(tBody.vVel.fY + kPhysicsDef.fGravity * fDt, kPhysicsDef.fMaxFallSpeed);

    // 축 분리: X 먼저 이동·해결, 그다음 Y (모서리 끼임 방지)
    tBody.vPos.fX += tBody.vVel.fX * fDt;
    ResolveX(tBody, tMap);

    tBody.bGrounded = false;
    tBody.vPos.fY += tBody.vVel.fY * fDt;
    ResolveY(tBody, tMap);
}
