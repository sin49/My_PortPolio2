#include "systems/Stats_U.h"

#include <algorithm>

namespace
{
float& GetField(StatBlock& tBlock, StatType eStat)
{
    switch (eStat)
    {
    case StatType::MaxHp:
        return tBlock.fMaxHp;
    case StatType::AttackDamage:
        return tBlock.fAttackDamage;
    case StatType::MoveSpeed:
        return tBlock.fMoveSpeed;
    case StatType::AttackRange:
        return tBlock.fAttackRange;
    }
    return tBlock.fMaxHp;  // 도달 안 함 (모든 경로에 반환이 있어야 /W4 경고 없음)
}
} // namespace

StatBlock Stats::Calculate(const StatBlock& tBase, std::span<const StatModifier> spMods)
{
    // 합계용은 전부 0 (StatBlock{}은 fAttackRange가 1.0이라 직접 지정)
    StatBlock tAdd = {.fMaxHp = 0.0f, .fAttackDamage = 0.0f, .fMoveSpeed = 0.0f, .fAttackRange = 0.0f};
    StatBlock tMul = tAdd;

    for (const StatModifier& tMod : spMods)
    {
        if (tMod.eOp == ModOp::Add)
            GetField(tAdd, tMod.eStat) += tMod.fValue;
        else
            GetField(tMul, tMod.eStat) += tMod.fValue;
    }

    StatBlock tFinal = tBase;
    constexpr StatType kAll[] = {StatType::MaxHp, StatType::AttackDamage, StatType::MoveSpeed, StatType::AttackRange};
    for (StatType eStat : kAll)
    {
        float& fOut = GetField(tFinal, eStat);
        fOut = (fOut + GetField(tAdd, eStat)) * (1.0f + GetField(tMul, eStat));
        fOut = std::max(0.0f, fOut);
    }
    return tFinal;
}
