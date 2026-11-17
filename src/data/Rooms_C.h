#pragma once

#include "core/Rect_C.h"
#include "core/Tilemap_C.h"
#include "core/Vec2_C.h"
#include "data/EnemyDef_C.h"

#include <array>
#include <span>
#include <string_view>

// 방 데이터 (docs/design/room-flow-ui.md). 넓은 방은 문자열 대신 "크기 + 벽 사각형 목록"으로 적음.
// TODO(확장): assets/rooms/*.txt 로드
enum class SpawnKind
{
    Dummy,    // 튜토리얼 허수아비 (공격 안 함)
    Chaser,
    Shooter,  // TODO: 사용자 Entity에 Shooter 추가 후. 지금은 Chaser로 대체 생성
    Boss,     // TODO: 보스 구현 후. 지금은 Chaser로 대체 생성
};

struct EnemySpawn
{
    SpawnKind eKind = SpawnKind::Chaser;
    float fTileX = 0.0f;  // 바닥 위 x (타일 단위). 높이는 바닥에 맞춰 생성
    EnemySize eSize = EnemySize::Small;  // 1·2·3대 (보스는 무시)
};

// 화면에 띄울 조작 안내 (월드 좌표). 키캡 + 그 아래 짧은 동작 설명
struct RoomHint
{
    float fTileX = 0.0f;
    float fTileY = 0.0f;
    std::string_view svKeys;    // 키캡 안 (예: "SHIFT")
    std::string_view svAction;  // 키캡 아래 작은 글씨 (예: "tap = dash")
};

struct RoomDef
{
    std::string_view svName;
    int iCols = 40;
    int iRows = 22;
    std::span<const TileRect> spSolids;
    std::span<const EnemySpawn> spEnemies;
    std::span<const RoomHint> spHints;
    bool bBossRoom = false;  // 보스 처치 = 승리. 문 없음
};

namespace rooms_detail
{
// ---------- 튜토리얼 70 x 22: 이동 -> 점프(턱) -> 달리기·대시(긴 복도) -> 공격(허수아비) ----------
inline constexpr std::array<TileRect, 4> kTutorialSolids = {{
    {16, 18, 3, 3},   // 점프 턱 (3칸)
    {28, 15, 4, 1},   // 발판
    {34, 12, 4, 1},   // 높은 발판 (달리며 점프)
    {48, 18, 2, 3},   // 낮은 기둥
}};
inline constexpr std::array<EnemySpawn, 1> kTutorialEnemies = {{{SpawnKind::Dummy, 58.0f}}};
inline constexpr std::array<RoomHint, 6> kTutorialHints = {{
    {4.0f, 14.0f, "<-  ->", "move"},
    {12.0f, 13.0f, "Z", "jump"},
    {22.0f, 9.0f, "Z", "hold = higher"},
    {38.0f, 14.0f, "SHIFT", "tap = dash (invincible)"},
    {38.0f, 17.0f, "SHIFT", "hold = run, jump higher"},
    {54.0f, 14.0f, "X", "attack"},
}};

// ---------- 방 1: 100 x 22, Chaser 4 ----------
inline constexpr std::array<TileRect, 8> kRoom1Solids = {{
    {12, 17, 6, 1},
    {24, 14, 6, 1},
    {36, 17, 8, 1},
    {44, 18, 2, 3},   // 기둥
    {50, 12, 6, 1},
    {62, 16, 8, 1},
    {78, 13, 6, 1},
    {88, 17, 5, 1},
}};
inline constexpr std::array<EnemySpawn, 7> kRoom1Enemies = {{
    {SpawnKind::Chaser, 20.0f},
    {SpawnKind::Chaser, 26.0f},
    {SpawnKind::Chaser, 40.0f, EnemySize::Medium},
    {SpawnKind::Chaser, 56.0f},
    {SpawnKind::Chaser, 66.0f},
    {SpawnKind::Chaser, 74.0f},
    {SpawnKind::Chaser, 86.0f, EnemySize::Large},
}};

// ---------- 방 2: 120 x 30 (세로로도 넓음), Chaser + Shooter ----------
inline constexpr std::array<TileRect, 12> kRoom2Solids = {{
    {8, 25, 8, 1},
    {20, 22, 6, 1},
    {30, 19, 6, 1},
    {40, 16, 10, 1},
    {54, 20, 6, 1},
    {56, 25, 2, 4},   // 기둥
    {64, 13, 8, 1},
    {76, 17, 6, 1},
    {86, 21, 8, 1},
    {90, 12, 6, 1},
    {100, 18, 6, 1},
    {108, 24, 6, 1},
}};
inline constexpr std::array<EnemySpawn, 10> kRoom2Enemies = {{
    {SpawnKind::Chaser, 18.0f},
    {SpawnKind::Chaser, 24.0f},
    {SpawnKind::Shooter, 44.0f},
    {SpawnKind::Chaser, 50.0f, EnemySize::Medium},
    {SpawnKind::Chaser, 62.0f},
    {SpawnKind::Chaser, 70.0f},
    {SpawnKind::Chaser, 80.0f, EnemySize::Large},
    {SpawnKind::Shooter, 96.0f},
    {SpawnKind::Chaser, 104.0f, EnemySize::Medium},
    {SpawnKind::Chaser, 110.0f},
}};

// ---------- 보스 방: 50 x 22 ----------
inline constexpr std::array<TileRect, 3> kBossSolids = {{
    {6, 16, 6, 1},
    {38, 16, 6, 1},
    {20, 12, 10, 1},
}};
inline constexpr std::array<EnemySpawn, 1> kBossEnemies = {{{SpawnKind::Boss, 34.0f}}};
} // namespace rooms_detail

inline constexpr std::array<RoomDef, 4> kRooms = {{
    {.svName = "Tutorial", .iCols = 70, .iRows = 22, .spSolids = rooms_detail::kTutorialSolids,
     .spEnemies = rooms_detail::kTutorialEnemies, .spHints = rooms_detail::kTutorialHints},
    {.svName = "Room 1", .iCols = 100, .iRows = 22, .spSolids = rooms_detail::kRoom1Solids,
     .spEnemies = rooms_detail::kRoom1Enemies},
    {.svName = "Room 2", .iCols = 120, .iRows = 30, .spSolids = rooms_detail::kRoom2Solids,
     .spEnemies = rooms_detail::kRoom2Enemies},
    {.svName = "Boss", .iCols = 50, .iRows = 22, .spSolids = rooms_detail::kBossSolids,
     .spEnemies = rooms_detail::kBossEnemies, .bBossRoom = true},
}};

// 방 공통 위치 계산 (px)
inline constexpr float GetRoomFloorTop(const RoomDef& tRoom)
{
    return static_cast<float>(tRoom.iRows - 1) * kTileSize;
}
inline constexpr Vec2 GetRoomPlayerSpawn(const RoomDef& tRoom)
{
    return Vec2{2.0f * kTileSize, GetRoomFloorTop(tRoom) - 48.0f};
}
// 문: 오른쪽 벽 바로 앞, 바닥 위 3칸 높이
inline constexpr Rect GetRoomDoor(const RoomDef& tRoom)
{
    return Rect{static_cast<float>(tRoom.iCols - 3) * kTileSize, GetRoomFloorTop(tRoom) - 3.0f * kTileSize,
                2.0f * kTileSize, 3.0f * kTileSize};
}
