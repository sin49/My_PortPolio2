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

Tilemap Tilemap::FromSolids(int iCols, int iRows, std::span<const TileRect> spSolids)
{
    Tilemap tMap;
    tMap.m_iCols = iCols;
    tMap.m_iRows = iRows;
    tMap.m_vecTiles.assign(static_cast<std::size_t>(iCols) * static_cast<std::size_t>(iRows), TileType::Empty);

    auto fnFill = [&](const TileRect& rcTiles) {
        for (int iY = rcTiles.iY; iY < rcTiles.iY + rcTiles.iH; ++iY)
            for (int iX = rcTiles.iX; iX < rcTiles.iX + rcTiles.iW; ++iX)
                if (iX >= 0 && iY >= 0 && iX < iCols && iY < iRows)
                    tMap.m_vecTiles[static_cast<std::size_t>(iY) * static_cast<std::size_t>(iCols) +
                                    static_cast<std::size_t>(iX)] = TileType::Solid;
    };

    fnFill({0, 0, iCols, 1});          // 천장
    fnFill({0, iRows - 1, iCols, 1});  // 바닥
    fnFill({0, 0, 1, iRows});          // 왼쪽 벽
    fnFill({iCols - 1, 0, 1, iRows});  // 오른쪽 벽
    for (const TileRect& rcTiles : spSolids)
        fnFill(rcTiles);
    return tMap;
}

TileType Tilemap::GetTile(int iTileX, int iTileY) const
{
    if (iTileX < 0 || iTileY < 0 || iTileX >= m_iCols || iTileY >= m_iRows)
        return TileType::Solid;
    return m_vecTiles[static_cast<std::size_t>(iTileY) * static_cast<std::size_t>(m_iCols) +
                      static_cast<std::size_t>(iTileX)];
}
