#include "systems/Items_C.h"

#include "core/World_U.h"

#include <algorithm>

Items::Items(std::uint32_t uSeed) : m_tRng(uSeed)
{
    m_vecDrops.reserve(16);
}

void Items::Reset()
{
    m_arrStacks = {};
    m_vecDrops.clear();
}

Handle Items::SpawnDrop(World& tWorld, Vec2 vCenter, ItemId eItem)
{
    if (tWorld.GetAliveCount() >= static_cast<int>(World::kCapacity))
        return Handle{};  // 풀이 꽉 참 (스트레스 테스트) -> 드랍 생략
    Entity tDrop;
    tDrop.eType = EntityType::Drop;
    tDrop.tBody = {.vPos = {vCenter.fX - kDropDef.vSize.fX * 0.5f, vCenter.fY - kDropDef.vSize.fY * 0.5f},
                   .vSize = kDropDef.vSize,
                   .vVel = {0.0f, -kDropDef.fPopSpeed}};
    tDrop.fHp = 1.0f;  // 의미 없음 (0이면 "삭제 예약 중"으로 보는 코드가 있어서 1)
    const Handle hDrop = tWorld.Create(tDrop);
    m_vecDrops.push_back({.hDrop = hDrop, .eItem = eItem, .fPickupDelay = kDropDef.fPickupDelay});
    return hDrop;
}

ItemId Items::SpawnClearReward(World& tWorld, Vec2 vCenter)
{
    std::uniform_int_distribution<std::size_t> tPick(0, kClearRewardItems.size() - 1);
    const ItemId eItem = kClearRewardItems[tPick(m_tRng)];
    SpawnDrop(tWorld, vCenter, eItem);
    return eItem;
}

float Items::GetPotionChance(float fHpRatio)
{
    if (fHpRatio >= 1.0f)
        return 0.0f;
    const float fMissing = 1.0f - std::max(0.0f, fHpRatio);
    return kPotionDropDef.fChanceMin + kPotionDropDef.fChanceByMissing * fMissing;
}

bool Items::TrySpawnPotion(World& tWorld, Vec2 vCenter, float fHpRatio)
{
    std::uniform_real_distribution<float> tRoll(0.0f, 1.0f);
    if (tRoll(m_tRng) >= GetPotionChance(fHpRatio))
        return false;
    SpawnDrop(tWorld, vCenter, ItemId::Potion);
    return true;
}

void Items::Update(World& tWorld, Handle hPlayer, float fMaxHp, float fDt, std::vector<PickupReport>& vecOutPickups)
{
    // 방이 바뀌며 지워진 드랍 = 세대가 달라진 핸들 -> Get이 nullptr (따로 알려 줄 필요 없음)
    std::erase_if(m_vecDrops, [&](const DropEntry& tEntry) { return tWorld.Get(tEntry.hDrop) == nullptr; });

    Entity* pPlayer = tWorld.Get(hPlayer);
    if (pPlayer == nullptr || pPlayer->fHp <= 0.0f)
        return;
    const Rect rcPlayer = GetBounds(pPlayer->tBody);

    for (DropEntry& tEntry : m_vecDrops)
    {
        tEntry.fPickupDelay -= fDt;
        if (tEntry.fPickupDelay > 0.0f)
            continue;
        const Entity* pDrop = tWorld.Get(tEntry.hDrop);
        if (pDrop == nullptr || !Overlaps(rcPlayer, GetBounds(pDrop->tBody)))
            continue;

        const ItemDef& tDef = GetItemDef(tEntry.eItem);
        if (tDef.fHealOnPickup > 0.0f)
            pPlayer->fHp = std::min(pPlayer->fHp + tDef.fHealOnPickup, fMaxHp);
        else
            ++m_arrStacks[static_cast<std::size_t>(tEntry.eItem)];

        const Rect rcDrop = GetBounds(pDrop->tBody);
        vecOutPickups.push_back({.eItem = tEntry.eItem, .vPos = {rcDrop.fX + rcDrop.fW * 0.5f, rcDrop.fY + rcDrop.fH * 0.5f}});
        tWorld.Destroy(tEntry.hDrop);  // 예약만 (Flush는 틱 끝)
        tEntry.bTaken = true;
    }
    // 순회가 끝난 뒤 한 번에 지움 (순회 중 삭제 금지)
    std::erase_if(m_vecDrops, [](const DropEntry& tEntry) { return tEntry.bTaken; });
}

void Items::BuildModifiers(std::vector<StatModifier>& vecOut) const
{
    vecOut.clear();
    for (std::size_t i = 0; i < kItemCount; ++i)
        for (int iStack = 0; iStack < m_arrStacks[i]; ++iStack)
            vecOut.insert(vecOut.end(), kItemDefs[i].spPerStack.begin(), kItemDefs[i].spPerStack.end());
}

const DropEntry* Items::FindDrop(Handle hDrop) const
{
    const auto it = std::find_if(m_vecDrops.begin(), m_vecDrops.end(),
                                 [&](const DropEntry& tEntry) { return tEntry.hDrop == hDrop; });
    return it == m_vecDrops.end() ? nullptr : &*it;
}
