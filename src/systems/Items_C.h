#pragma once
// 아이템 드랍·획득·스택 (docs/design/items.md). 스탯을 직접 바꾸지 않음 -> modifier 목록만 제공 (GDD 7장)
#include "core/Handle_U.h"
#include "core/Vec2_C.h"
#include "data/ItemDefs_C.h"

#include <array>
#include <cstdint>
#include <random>
#include <vector>

class World;

// 바닥에 떨어진 아이템 1개. Drop 엔티티는 핸들로만 기억 (방이 바뀌어 지워지면 Get = nullptr)
struct DropEntry
{
    Handle hDrop;
    ItemId eItem = ItemId::Potion;
    float fPickupDelay = 0.0f;
    bool bTaken = false;  // 이번 Update에서 주움 -> 순회가 끝난 뒤 목록에서 지움
};

// 주운 것 1건 (연출·알림용)
struct PickupReport
{
    ItemId eItem = ItemId::Potion;
    Vec2 vPos = {};
};

class Items
{
public:
    explicit Items(std::uint32_t uSeed = 20261119u);  // 시드 고정 가능 -> 테스트 재현

    void Reset();  // 새 게임: 스택·드랍 목록 비움

    // vCenter에서 위로 튀며 떨어지는 Drop 엔티티 생성
    Handle SpawnDrop(World& tWorld, Vec2 vCenter, ItemId eItem);
    // 방 클리어 보상: 스탯 아이템 4종 중 랜덤 1개
    ItemId SpawnClearReward(World& tWorld, Vec2 vCenter);
    // 적이 죽을 때: HP에 따른 확률로 회복약. 떨어뜨렸으면 true
    bool TrySpawnPotion(World& tWorld, Vec2 vCenter, float fHpRatio);
    static float GetPotionChance(float fHpRatio);

    // 몸이 닿은 드랍 줍기. 회복약은 즉시 회복(최대 fMaxHp), 나머지는 스택 +1
    void Update(World& tWorld, Handle hPlayer, float fMaxHp, float fDt, std::vector<PickupReport>& vecOutPickups);

    // Stats에 넘길 목록: 아이템마다 perStack을 스택 수만큼 (호출자 버퍼 재사용)
    void BuildModifiers(std::vector<StatModifier>& vecOut) const;

    int GetStack(ItemId eItem) const { return m_arrStacks[static_cast<std::size_t>(eItem)]; }
    const DropEntry* FindDrop(Handle hDrop) const;  // Render가 색을 고를 때. 없으면 nullptr

private:
    std::array<int, kItemCount> m_arrStacks = {};
    std::vector<DropEntry> m_vecDrops;
    std::mt19937 m_tRng;
};
