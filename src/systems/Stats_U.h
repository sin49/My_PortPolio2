#pragma once
// 스탯 = 기본값 + modifier 재계산 (docs/design/stats.md). 원본은 절대 안 바뀜
#include <span>

enum class StatType
{
    MaxHp,
    AttackDamage,
    MoveSpeed,
    AttackRange
};

enum class ModOp
{
    Add,
    Mul
};

struct StatModifier
{
    StatType eStat = StatType::MaxHp;
    ModOp eOp = ModOp::Add;
    float fValue = 0.0f;  // Add: 그대로 더함 / Mul: 비율 (0.15 = +15%)
};

struct StatBlock
{
    float fMaxHp = 0.0f;
    float fAttackDamage = 0.0f;
    float fMoveSpeed = 0.0f;
    float fAttackRange = 1.0f;  // 배율. 1.0 = 기본 히트박스 크기
};

class Stats
{
public:
    static StatBlock Calculate(const StatBlock& tBase, std::span<const StatModifier> spMods);
};
