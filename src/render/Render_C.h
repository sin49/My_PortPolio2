#pragma once

#include "core/Handle_U.h"
#include "core/Vec2_C.h"
#include "data/ItemDefs_C.h"

class Items;
class ViewCamera;
class Particles;
class RoomFlow;
class Tilemap;
class World;

inline constexpr int kScreenWidth = 1280;
inline constexpr int kScreenHeight = 720;
// 방이 보이는 영역: 1280 x 704 (위·아래 8px 검은 프레임)
inline constexpr int kViewHeight = 704;
inline constexpr int kFrameOffsetY = (kScreenHeight - kViewHeight) / 2;
inline constexpr Vec2 kViewSize = {static_cast<float>(kScreenWidth), static_cast<float>(kViewHeight)};

// 루프 관련 디버그 값 (게임 상태가 아닌 것). Game이 채워서 넘김.
struct DebugInfo
{
    int iStepsThisFrame = 0;
    long long llTotalTicks = 0;
    bool bShow = false;  // F1
};

// 아이템 획득 알림 (연출 상태, Game이 관리)
struct PickupNotice
{
    ItemId eItem = ItemId::Potion;
    float fTimer = 0.0f;  // > 0 이면 표시 중
};

// 한 프레임에 그릴 것 전부 (읽기 전용 묶음)
struct FrameView
{
    const Tilemap& tRoom;
    const World& tWorld;
    Handle hPlayer;
    const Particles& tParticles;
    const ViewCamera& tCamera;
    const RoomFlow& tRoomFlow;
    const Items& tItems;
    const StatBlock& tPlayerStats;  // 플레이어 최종 스탯 (최대 HP·공격력 표시용)
    PickupNotice tPickupNotice;
    DebugInfo tDebug;
};

// 그리기만 함. 게임 상태는 const&로 읽기만.
// 방 좌표 -> 화면 좌표 변환(카메라·프레임 오프셋)은 Render만 앎.
class Render
{
public:
    void Draw(const FrameView& tView) const;
};
