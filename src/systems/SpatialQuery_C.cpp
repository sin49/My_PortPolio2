#include "systems/SpatialQuery_C.h"

#include "core/World_U.h"

void SpatialQuery::QueryNearby(const Rect& rcArea, std::vector<Handle>& vecOut) const
{
    vecOut.clear();
    m_tWorld.ForEachAlive([&](Handle hEntity, const Entity& tEntity) {
        if (Overlaps(rcArea, GetBounds(tEntity.tBody)))
            vecOut.push_back(hEntity);
    });
}
