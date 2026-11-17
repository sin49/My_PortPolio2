// Stats 스펙 테스트 (AI 작성). 사용자가 구현한 Stats가 이 테스트를 통과하면 완료.
// 기준: docs/design/stats.md. 테스트가 설계와 다르다고 생각되면 고치지 말고 먼저 논의.
#include "systems/Stats.h"

#include <array>
#include <vector>

#include <doctest/doctest.h>

namespace
{
constexpr StatBlock kBase = {.fMaxHp = 100.0f, .fAttackDamage = 10.0f, .fMoveSpeed = 260.0f, .fAttackRange = 1.0f};
} // namespace

TEST_CASE("Stats: modifier 없으면 기본값 그대로")
{
    const std::vector<StatModifier> vecNone;
    const StatBlock tFinal = Stats::Calculate(kBase, vecNone);
    CHECK(tFinal.fMaxHp == doctest::Approx(100.0f));
    CHECK(tFinal.fAttackDamage == doctest::Approx(10.0f));
    CHECK(tFinal.fMoveSpeed == doctest::Approx(260.0f));
    CHECK(tFinal.fAttackRange == doctest::Approx(1.0f));
}

TEST_CASE("Stats: Add는 더하고 Mul은 비율")
{
    const std::array<StatModifier, 2> arrMods = {{
        {.eStat = StatType::MaxHp, .eOp = ModOp::Add, .fValue = 20.0f},
        {.eStat = StatType::MoveSpeed, .eOp = ModOp::Mul, .fValue = 0.5f},
    }};
    const StatBlock tFinal = Stats::Calculate(kBase, arrMods);
    CHECK(tFinal.fMaxHp == doctest::Approx(120.0f));
    CHECK(tFinal.fMoveSpeed == doctest::Approx(390.0f));     // 260 * 1.5
    CHECK(tFinal.fAttackDamage == doctest::Approx(10.0f));   // 안 건드린 스탯은 그대로
}

TEST_CASE("Stats: Add 먼저 전부, Mul 나중에 (순서 무관)")
{
    // 목록 순서를 Mul이 앞에 오게 해도 결과는 (10 + 5) * 1.2 = 18
    const std::array<StatModifier, 2> arrMods = {{
        {.eStat = StatType::AttackDamage, .eOp = ModOp::Mul, .fValue = 0.2f},
        {.eStat = StatType::AttackDamage, .eOp = ModOp::Add, .fValue = 5.0f},
    }};
    CHECK(Stats::Calculate(kBase, arrMods).fAttackDamage == doctest::Approx(18.0f));
}

TEST_CASE("Stats: Mul끼리는 더함 (곱하지 않음)")
{
    // +50% 두 개 = +100% -> 2배 (곱하면 2.25배)
    const std::array<StatModifier, 2> arrMods = {{
        {.eStat = StatType::AttackRange, .eOp = ModOp::Mul, .fValue = 0.5f},
        {.eStat = StatType::AttackRange, .eOp = ModOp::Mul, .fValue = 0.5f},
    }};
    CHECK(Stats::Calculate(kBase, arrMods).fAttackRange == doctest::Approx(2.0f));
}

TEST_CASE("Stats: 같은 아이템 여러 개(스택) = 같은 modifier 여러 번")
{
    std::vector<StatModifier> vecMods;
    for (int i = 0; i < 3; ++i)
        vecMods.push_back({.eStat = StatType::MaxHp, .eOp = ModOp::Add, .fValue = 10.0f});
    CHECK(Stats::Calculate(kBase, vecMods).fMaxHp == doctest::Approx(130.0f));
}

TEST_CASE("Stats: 원본(기본값)은 안 바뀜 - 덮어쓰기 금지")
{
    StatBlock tBase = kBase;
    const std::array<StatModifier, 1> arrMods = {{{.eStat = StatType::MaxHp, .eOp = ModOp::Add, .fValue = 50.0f}}};
    Stats::Calculate(tBase, arrMods);
    Stats::Calculate(tBase, arrMods);  // 두 번 계산해도
    CHECK(tBase.fMaxHp == doctest::Approx(100.0f));
    CHECK(Stats::Calculate(tBase, arrMods).fMaxHp == doctest::Approx(150.0f));  // 누적되지 않음
}

TEST_CASE("Stats: 최종값은 0 미만이 안 됨")
{
    const std::array<StatModifier, 1> arrMods = {{{.eStat = StatType::MoveSpeed, .eOp = ModOp::Mul, .fValue = -3.0f}}};
    CHECK(Stats::Calculate(kBase, arrMods).fMoveSpeed == doctest::Approx(0.0f));
}
