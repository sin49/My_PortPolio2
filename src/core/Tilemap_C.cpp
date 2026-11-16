#include "core/Tilemap_C.h"

#include <cstddef>

std::optional<Tilemap> Tilemap::FromRows(std::span<const std::string_view> spRowTexts)
{
    if (spRowTexts.empty() || spRowTexts.front().empty())
        return std::nullopt;

    Tilemap tMap;
    tMap.m_iCols = static_cast<int>(spRowTexts.front().size());
    tMap.m_iRows = static_cast<int>(spRowTexts.size());
    tMap.m_vecTiles.reserve(static_cast<std::size_t>(tMap.m_iCols) * static_cast<std::size_t>(tMap.m_iRows));

    for (std::string_view svRow : spRowTexts)
    {
        if (static_cast<int>(svRow.size()) != tMap.m_iCols)
            return std::nullopt;

        for (char cTile : svRow)
        {
            switch (cTile)
            {
            case '#': tMap.m_vecTiles.push_back(TileType::Solid); break;
            case '.': tMap.m_vecTiles.push_back(TileType::Empty); break;
            default: return std::nullopt;
            }
        }
    }
    return tMap;
}

TileType Tilemap::GetTile(int iTileX, int iTileY) const
{
    if (iTileX < 0 || iTileY < 0 || iTileX >= m_iCols || iTileY >= m_iRows)
        return TileType::Solid;
    return m_vecTiles[static_cast<std::size_t>(iTileY) * static_cast<std::size_t>(m_iCols) +
                      static_cast<std::size_t>(iTileX)];
}
