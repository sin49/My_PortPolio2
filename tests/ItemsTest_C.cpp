// 아이템 드랍·획득·스택 + Stats 연결 테스트 (docs/design/items.md 6장)
#include "core/FixedTimestep_C.h"
#include "core/World_U.h"
#include "data/EntityFactory_C.h"
#include "data/ItemDefs_C.h"
#include "systems/Items_C.h"
#include "systems/Stats_U.h"

#include <ostream>  // doctest가 실패 시 string_view를 출력할 때 필요
#include <vector>

#include <doctest/doctest.h>

namespace
{
constexpr float kDt = FixedTimestep::kStep;

struct Bench
{
    World tWorld;
    Items tItems{7u};
    std::vector<PickupReport> vecPickups;
    Handle hPlayer = tWorld.Create(MakePlayerEntity({100.0f, 100.0f}));

    Vec2 PlayerCenter() const
    {
        const Body& tBody = tWorld.Get(hPlayer)->tBody;
        return {tBody.vPos.fX + tBody.vSize.fX * 0.5f, tBody.vPos.fY + tBody.vSize.fY * 0.5f};
    }

    // 드랍 대기시간이 지날 만큼 Update (드랍은 Physics를 안 돌려서 제자리)
    void Wait(float fSeconds, float fMaxHp = kPlayerBaseStats.fMaxHp)
    {
        for (float fT = 0.0f; fT < fSeconds; fT += kDt)
        {
            tItems.Update(tWorld, hPlayer, fMaxHp, kDt, vecPickups);
            tWorld.FlushRemovals();
        }
    }
};
} // namespace

TEST_CASE("Items: 테이블 순서가 ItemId와 같음")
{
    CHECK(GetItemDef(ItemId::Potion).svName == "Potion");
    CHECK(GetItemDef(ItemId::Heart).svName == "Heart");
    CHECK(GetItemDef(ItemId::Whetstone).svName == "Whetstone");
    CHECK(GetItemDef(ItemId::Feather).svName == "Feather");
    CHECK(GetItemDef(ItemId::LongBlade).svName == "Long Blade");
}

TEST_CASE("Items: 몸이 닿으면 스택 +1, 드랍 삭제 / 대기시간 전엔 못 주움")
{
    Bench tBench;
    const Handle hDrop = tBench.tItems.SpawnDrop(tBench.tWorld, tBench.PlayerCenter(), ItemId::Feather);
    tBench.Wait(kDropDef.fPickupDelay * 0.5f);
    CHECK(tBench.tItems.GetStack(ItemId::Feather) == 0);  // 아직 튀는 중

    tBench.Wait(kDropDef.fPickupDelay);
    CHECK(tBench.tItems.GetStack(ItemId::Feather) == 1);
    CHECK(tBench.tWorld.Get(hDrop) == nullptr);
    CHECK(tBench.tItems.FindDrop(hDrop) == nullptr);
}

TEST_CASE("Items: 멀리 있으면 안 주워짐")
{
    Bench tBench;
    const Handle hDrop = tBench.tItems.SpawnDrop(tBench.tWorld, {600.0f, 100.0f}, ItemId::Heart);
    tBench.Wait(1.0f);
    CHECK(tBench.tItems.GetStack(ItemId::Heart) == 0);
    CHECK(tBench.tItems.FindDrop(hDrop) != nullptr);
}

TEST_CASE("Items: 회복약은 즉시 회복, 최대 HP에서 멈춤, 인벤토리에 안 남음")
{
    Bench tBench;
    tBench.tWorld.Get(tBench.hPlayer)->fHp = 50.0f;
    tBench.tItems.SpawnDrop(tBench.tWorld, tBench.PlayerCenter(), ItemId::Potion);
    tBench.Wait(1.0f);
    CHECK(tBench.tWorld.Get(tBench.hPlayer)->fHp == doctest::Approx(80.0f));
    CHECK(tBench.tItems.GetStack(ItemId::Potion) == 0);

    tBench.tItems.SpawnDrop(tBench.tWorld, tBench.PlayerCenter(), ItemId::Potion);
    tBench.Wait(1.0f);
    CHECK(tBench.tWorld.Get(tBench.hPlayer)->fHp == doctest::Approx(100.0f));  // 110이 아님
}

TEST_CASE("Items: 스택 -> modifier -> Stats::Calculate (깃털 2개 = 이동 속도 +24%)")
{
    Bench tBench;
    for (int i = 0; i < 2; ++i)
    {
        tBench.tItems.SpawnDrop(tBench.tWorld, tBench.PlayerCenter(), ItemId::Feather);
        tBench.Wait(1.0f);
    }
    tBench.tItems.SpawnDrop(tBench.tWorld, tBench.PlayerCenter(), ItemId::Whetstone);
    tBench.Wait(1.0f);

    std::vector<StatModifier> vecMods;
    tBench.tItems.BuildModifiers(vecMods);
    CHECK(vecMods.size() == 3);
    const StatBlock tFinal = Stats::Calculate(kPlayerBaseStats, vecMods);
    CHECK(tFinal.fMoveSpeed == doctest::Approx(kPlayerDef.fMoveSpeed * 1.24f));
    CHECK(tFinal.fAttackDamage == doctest::Approx(kPlayerDef.fAttackDamage + 5.0f));
    CHECK(tFinal.fAttackRange == doctest::Approx(1.0f));
    CHECK(kPlayerBaseStats.fMoveSpeed == doctest::Approx(kPlayerDef.fMoveSpeed));  // 원본 그대로

    tBench.tItems.Reset();  // 새 게임: 목록만 비우면 스탯도 원래대로 (되돌리기 코드 없음)
    tBench.tItems.BuildModifiers(vecMods);
    CHECK(Stats::Calculate(kPlayerBaseStats, vecMods).fMoveSpeed == doctest::Approx(kPlayerDef.fMoveSpeed));
}

TEST_CASE("Items: 드랍 엔티티가 밖에서 지워지면(방 전환) 목록에서도 빠짐")
{
    Bench tBench;
    const Handle hDrop = tBench.tItems.SpawnDrop(tBench.tWorld, {600.0f, 100.0f}, ItemId::Heart);
    tBench.tWorld.Destroy(hDrop);  // RoomFlow.LoadRoom이 하는 일
    tBench.tWorld.FlushRemovals();
    tBench.Wait(kDt);
    CHECK(tBench.tItems.FindDrop(hDrop) == nullptr);
}

TEST_CASE("Items: 회복약 확률은 HP에 따라 (가득 = 0%, 낮을수록 높음)")
{
    CHECK(Items::GetPotionChance(1.0f) == doctest::Approx(0.0f));
    CHECK(Items::GetPotionChance(0.5f) == doctest::Approx(0.225f));
    CHECK(Items::GetPotionChance(0.1f) == doctest::Approx(0.365f));
    CHECK(Items::GetPotionChance(0.1f) > Items::GetPotionChance(0.5f));

    // HP 가득이면 몇 번을 굴려도 안 나옴
    World tWorld;
    Items tItems{1u};
    int iSpawned = 0;
    for (int i = 0; i < 200; ++i)
        iSpawned += tItems.TrySpawnPotion(tWorld, {0.0f, 0.0f}, 1.0f) ? 1 : 0;
    CHECK(iSpawned == 0);
}

TEST_CASE("Items: 클리어 보상은 스탯 아이템 4종 중 하나 (회복약 X), 같은 시드면 같은 결과")
{
    World tWorldA;
    World tWorldB;
    Items tItemsA{42u};
    Items tItemsB{42u};
    for (int i = 0; i < 50; ++i)
    {
        const ItemId eA = tItemsA.SpawnClearReward(tWorldA, {0.0f, 0.0f});
        CHECK(eA != ItemId::Potion);
        CHECK(eA == tItemsB.SpawnClearReward(tWorldB, {0.0f, 0.0f}));
    }
}
