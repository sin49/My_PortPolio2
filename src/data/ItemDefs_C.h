#pragma once
// 아이템 정의 테이블 (docs/design/items.md). 아이템 = "스택 1개당 modifier 목록" (GDD 8장)
#include "data/PlayerDef_C.h"
#include "systems/Stats_U.h"

#include <array>
#include <cstddef>
#include <span>
#include <string_view>

enum class ItemId
{
    Potion,     // 즉시 회복 (인벤토리에 안 남음, 적 처치 때 확률 드랍)
    Heart,      // 최대 HP
    Whetstone,  // 공격력
    Feather,    // 이동 속도
    LongBlade,  // 공격 범위
    Count,
};
inline constexpr std::size_t kItemCount = static_cast<std::size_t>(ItemId::Count);

struct ItemColor
{
    int iR = 255;
    int iG = 255;
    int iB = 255;
};

struct ItemDef
{
    std::string_view svName;           // 화면 표시 (raylib 기본 글꼴 = 영어만)
    ItemColor tColor;
    float fHealOnPickup = 0.0f;        // > 0 이면 즉시 회복 아이템 (스택 안 함)
    std::span<const StatModifier> spPerStack;  // 스택 1개당 효과
};

// 플레이어 기본 스탯 (원본. constexpr이라 아무도 못 바꿈 = 덮어쓰기 금지가 타입으로 보장)
inline constexpr StatBlock kPlayerBaseStats = {.fMaxHp = kPlayerDef.fMaxHp,
                                               .fAttackDamage = kPlayerDef.fAttackDamage,
                                               .fMoveSpeed = kPlayerDef.fMoveSpeed,
                                               .fAttackRange = 1.0f};

inline constexpr std::array<StatModifier, 1> kHeartMods = {{{.eStat = StatType::MaxHp, .eOp = ModOp::Add, .fValue = 20.0f}}};
inline constexpr std::array<StatModifier, 1> kWhetstoneMods = {
    {{.eStat = StatType::AttackDamage, .eOp = ModOp::Add, .fValue = 5.0f}}};
inline constexpr std::array<StatModifier, 1> kFeatherMods = {
    {{.eStat = StatType::MoveSpeed, .eOp = ModOp::Mul, .fValue = 0.12f}}};
inline constexpr std::array<StatModifier, 1> kLongBladeMods = {
    {{.eStat = StatType::AttackRange, .eOp = ModOp::Mul, .fValue = 0.25f}}};

// ItemId 순서와 같아야 함 (테스트가 이름으로 확인)
inline constexpr std::array<ItemDef, kItemCount> kItemDefs = {{
    {.svName = "Potion", .tColor = {225, 70, 80}, .fHealOnPickup = 30.0f, .spPerStack = {}},
    {.svName = "Heart", .tColor = {240, 130, 190}, .fHealOnPickup = 0.0f, .spPerStack = kHeartMods},
    {.svName = "Whetstone", .tColor = {240, 150, 60}, .fHealOnPickup = 0.0f, .spPerStack = kWhetstoneMods},
    {.svName = "Feather", .tColor = {120, 200, 245}, .fHealOnPickup = 0.0f, .spPerStack = kFeatherMods},
    {.svName = "Long Blade", .tColor = {240, 220, 90}, .fHealOnPickup = 0.0f, .spPerStack = kLongBladeMods},
}};

// 방 클리어 보상 후보 (회복약 제외, 사용자 결정 11/19)
inline constexpr std::array<ItemId, 4> kClearRewardItems = {ItemId::Heart, ItemId::Whetstone, ItemId::Feather,
                                                            ItemId::LongBlade};

// 회복약 드랍 확률 = HP 가득이면 0, 아니면 fChanceMin + fChanceByMissing × (잃은 HP 비율)
struct PotionDropDef
{
    float fChanceMin = 0.05f;
    float fChanceByMissing = 0.35f;
};
inline constexpr PotionDropDef kPotionDropDef = {};

struct DropDef
{
    Vec2 vSize = {14.0f, 14.0f};
    float fPopSpeed = 360.0f;     // 생길 때 위로 튀는 속도
    float fPickupDelay = 0.35f;   // 생긴 뒤 이 시간 동안은 못 주움 (튀는 게 보이게)
};
inline constexpr DropDef kDropDef = {};

inline const ItemDef& GetItemDef(ItemId eItem)
{
    return kItemDefs[static_cast<std::size_t>(eItem)];
}
