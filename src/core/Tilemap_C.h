#pragma once

#include <optional>
#include <span>
#include <string_view>
#include <vector>

inline constexpr float kTileSize = 32.0f;
inline constexpr int kRoomCols = 40;  // 1280 / 32
inline constexpr int kRoomRows = 22;  // 704 / 32 (창 720에서 위·아래 8px은 프레임)

enum class TileType : unsigned char
{
    Empty,
    Solid,
};

// 방 하나의 타일 격자. 문자 '#' = Solid, '.' = Empty.
class Tilemap
{
public:
    // 행 문자열 배열로 생성. 행 길이가 다르거나 모르는 문자가 있으면 nullopt.
    static std::optional<Tilemap> FromRows(std::span<const std::string_view> spRowTexts);

    int GetCols() const { return m_iCols; }
    int GetRows() const { return m_iRows; }

    // 범위 밖은 Solid로 취급 (방 밖으로 못 나감)
    TileType GetTile(int iTileX, int iTileY) const;
    bool IsSolid(int iTileX, int iTileY) const { return GetTile(iTileX, iTileY) == TileType::Solid; }

private:
    int m_iCols = 0;
    int m_iRows = 0;
    std::vector<TileType> m_vecTiles;  // 행 우선: index = y * cols + x
};
