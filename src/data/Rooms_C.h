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
    Shooter,  // 거리 유지 + 1발 (ShooterAI_C)
    Boss,     // BossAI_C (조준 3연사 <-> 8방향)
};

struct EnemySpawn
{
    SpawnKind eKind = SpawnKind::Chaser;
    float fTileX = 0.0f;  // 발 가운데 x (타일 단위)
    EnemySize eSize = EnemySize::Small;  // 1·2·3대 (보스는 무시)
    int iStandTileY = 0;  // 서 있을 발판의 타일 y (발판 윗면). 0 = 바닥 (11/19: 위쪽 발판에도 적 배치)
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

// ---------- 방 1: 56 x 22 (11/19 2차: 72 -> 56, 위로 오르는 발판 계단 + 위쪽 적), Chaser 5 + Shooter 3 ----------
// 발판 높이 차이는 3~4칸 (점프 최고 약 5칸). iStandTileY = 그 발판의 y
inline constexpr std::array<TileRect, 13> kRoom1Solids = {{
    {6, 18, 5, 1},
    {13, 15, 4, 1},
    {6, 12, 5, 1},
    {13, 9, 4, 1},    // 왼쪽 계단 꼭대기
    {20, 18, 6, 1},
    {27, 18, 2, 3},   // 기둥
    {30, 15, 5, 1},
    {37, 12, 5, 1},
    {30, 9, 4, 1},
    {44, 15, 5, 1},
    {49, 12, 4, 1},
    {41, 8, 6, 1},    // 오른쪽 꼭대기
    {50, 18, 3, 1},
}};
inline constexpr std::array<EnemySpawn, 8> kRoom1Enemies = {{
    {SpawnKind::Chaser, 12.0f},
    {SpawnKind::Chaser, 8.5f, EnemySize::Small, 12},
    {SpawnKind::Shooter, 15.0f, EnemySize::Small, 9},
    {SpawnKind::Chaser, 23.0f, EnemySize::Medium, 18},
    {SpawnKind::Chaser, 32.5f, EnemySize::Small, 15},
    {SpawnKind::Shooter, 39.5f, EnemySize::Small, 12},
    {SpawnKind::Chaser, 44.0f, EnemySize::Large},
    {SpawnKind::Shooter, 44.0f, EnemySize::Small, 8},
}};

// ---------- 방 2: 64 x 26 (11/19 2차: 88 -> 64, 세로 탑 3개), Chaser 7 + Shooter 4 ----------
inline constexpr std::array<TileRect, 18> kRoom2Solids = {{
    {5, 22, 5, 1},    // 왼쪽 탑
    {12, 19, 4, 1},
    {5, 16, 5, 1},
    {12, 13, 4, 1},
    {5, 10, 5, 1},
    {12, 7, 5, 1},
    {20, 22, 6, 1},
    {27, 21, 2, 4},   // 기둥 (가운데 탑 디딤)
    {31, 19, 5, 1},   // 가운데 탑
    {38, 16, 5, 1},
    {31, 13, 5, 1},
    {38, 10, 5, 1},
    {30, 6, 7, 1},
    {46, 20, 5, 1},   // 오른쪽 탑
    {53, 17, 4, 1},
    {46, 13, 5, 1},
    {53, 9, 4, 1},
    {57, 22, 3, 1},
}};
inline constexpr std::array<EnemySpawn, 11> kRoom2Enemies = {{
    {SpawnKind::Chaser, 16.0f},
    {SpawnKind::Chaser, 7.5f, EnemySize::Small, 16},
    {SpawnKind::Shooter, 14.0f, EnemySize::Small, 13},
    {SpawnKind::Chaser, 14.5f, EnemySize::Medium, 7},
    {SpawnKind::Chaser, 24.0f, EnemySize::Medium},
    {SpawnKind::Shooter, 33.5f, EnemySize::Small, 13},
    {SpawnKind::Chaser, 40.5f, EnemySize::Small, 10},
    {SpawnKind::Shooter, 33.5f, EnemySize::Small, 6},
    {SpawnKind::Chaser, 48.5f, EnemySize::Large, 13},
    {SpawnKind::Shooter, 55.0f, EnemySize::Small, 9},
    {SpawnKind::Chaser, 50.0f},
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
    {.svName = "Room 1", .iCols = 56, .iRows = 22, .spSolids = rooms_detail::kRoom1Solids,
     .spEnemies = rooms_detail::kRoom1Enemies},
    {.svName = "Room 2", .iCols = 64, .iRows = 26, .spSolids = rooms_detail::kRoom2Solids,
     .spEnemies = rooms_detail::kRoom2Enemies},
    {.svName = "Boss", .iCols = 50, .iRows = 22, .spSolids = rooms_detail::kBossSolids,
     .spEnemies = rooms_detail::kBossEnemies, .bBossRoom = true},
}};

// 방 공통 위치 계산 (px)
inline constexpr float GetRoomFloorTop(const RoomDef& tRoom)
{
    return static_cast<float>(tRoom.iRows - 1) * kTileSize;
}
// 적 발 위치 (px): 발판 윗면 또는 바닥
inline constexpr Vec2 GetSpawnFeet(const RoomDef& tRoom, const EnemySpawn& tSpawn)
{
    const float fFeetY = tSpawn.iStandTileY > 0 ? static_cast<float>(tSpawn.iStandTileY) * kTileSize : GetRoomFloorTop(tRoom);
    return Vec2{tSpawn.fTileX * kTileSize, fFeetY};
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
