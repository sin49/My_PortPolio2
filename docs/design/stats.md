# Stats (기본값 + modifier 재계산) — 설계 (**사용자 구현**)

> 상태: **제안 (2026-11-17)**. 포폴 핵심이라 사용자가 직접 구현. AI는 의사코드·스펙 테스트·아이템 연결.
> 기준: GDD 8장, CLAUDE.md "스탯은 덮어쓰지 않음. 기본값 + modifier 재계산만"
> 핵심 아이디어: **최종값 = 계산 결과일 뿐, 저장된 원본(기본값)은 절대 안 바뀜.** 아이템을 잃으면 modifier 목록에서 빼고 다시 계산하면 끝 (되돌리기 코드 없음).

## 1. 결정 (AI 제안, 사용자 확인 필요)
| # | 질문 | AI 추천 |
|---|---|---|
| 1 | 계산 공식 | `최종 = (기본 + Add 합) × (1 + Mul 합)`. Mul은 비율(0.1 = +10%)이고 **서로 더함** (곱하면 쌓일수록 폭주) |
| 2 | 스탯 종류 | `MaxHp`, `AttackDamage`, `MoveSpeed`, `AttackRange`(공격 범위 배율, 기본 1.0) 4개 |
| 3 | 음수 | 최종값은 0 미만이 안 되게 (속도 -10% 아이템을 많이 먹어도 0에서 멈춤) |
| 4 | 모양 | 상태 없는 계산 함수 1개 (`Stats::Calculate`). modifier 목록 보관은 아이템 쪽(AI)이 함 → Stats는 테스트하기 제일 쉬운 순수 계산 |

## 2. 사용자 작업 — 이 장만 따라가면 됨
### 2-1. 만들 파일
| 파일 | 내용 |
|---|---|
| `src/systems/Stats.h` | enum 2개, 구조체 2개, `class Stats` 선언 |
| `src/systems/Stats.cpp` | `Calculate` + 도우미 함수 |

- 만든 뒤 `cmake -S . -B build` 한 번 → 출력 `Stats 구현 포함: ON` → 스펙 테스트 `tests/StatsTest_C.cpp`(7개)도 같이 돎. 전부 통과하면 완료.
- 이름(`StatType::MoveSpeed`, `tFinal.fMoveSpeed`, `Stats::Calculate`)은 테스트가 쓰므로 그대로. 나머지 이름은 자유 (리뷰 때 AI가 정리).
- **raylib include 금지** (GameLogic 라이브러리 = 테스트 가능한 로직).

### 2-2. Stats.h
```
#pragma once
#include <span>                          // [C++] std::span = "배열/벡터의 일부를 빌려 보는 창". C#의 ReadOnlySpan<T>

enum class StatType { MaxHp, AttackDamage, MoveSpeed, AttackRange }
enum class ModOp { Add, Mul }

struct StatModifier {
    StatType eStat = StatType::MaxHp
    ModOp eOp = ModOp::Add
    float fValue = 0.0f                  // Add: 그대로 더함 (+20 HP) / Mul: 비율 (0.15 = +15%)
}

struct StatBlock {                       // 스탯 4개 묶음 (기본값도, 최종값도 이 모양)
    float fMaxHp = 0.0f
    float fAttackDamage = 0.0f
    float fMoveSpeed = 0.0f
    float fAttackRange = 1.0f            // 배율. 1.0 = 기본 히트박스 크기
}

class Stats {
public:
    static StatBlock Calculate(const StatBlock& tBase, std::span<const StatModifier> spMods)
        // [C++] static = 객체 없이 Stats::Calculate(...)로 부름 (C#의 static 메서드와 같음)
        // [C++] tBase는 const& -> 복사 안 하고 읽기만. 결과는 "새 StatBlock"을 값으로 돌려줌 (원본 안 바뀜)
}
```

### 2-3. Stats.cpp
```
#include "systems/Stats.h"
#include <algorithm>                     // std::max

namespace {
    // StatType -> StatBlock 안의 해당 필드를 "참조로" 꺼냄. 이걸로 읽기·쓰기 둘 다 함
    GetField(StatBlock& tBlock, StatType eStat) -> float&
        // [C++] 반환형 float& = 필드 자체의 별명을 돌려줌. 받은 쪽에서 += 하면 tBlock 안의 값이 바뀜
        //       (C#엔 비슷한 게 ref return. 값 float를 돌려주면 복사본이라 안 바뀜 — 여기선 & 필수)
        switch eStat:
            case StatType::MaxHp:        return tBlock.fMaxHp
            case StatType::AttackDamage: return tBlock.fAttackDamage
            case StatType::MoveSpeed:    return tBlock.fMoveSpeed
            case StatType::AttackRange:  return tBlock.fAttackRange
        return tBlock.fMaxHp             // [C++] 모든 case에서 return해도 끝에 하나 있어야 /W4 경고 없음 (도달 안 함)
}

Stats::Calculate(const StatBlock& tBase, std::span<const StatModifier> spMods) -> StatBlock:
                                         // [C++] cpp에서는 "StatBlock Stats::Calculate(...)" 순서. static은 h에만 씀
    StatBlock tAdd = {}                  // Add 합계 (전부 0으로 시작하게: fAttackRange도 0으로 직접 맞출 것)
    StatBlock tMul = {}                  // Mul 합계 (마찬가지로 전부 0)
        // [C++] StatBlock{}은 기본값(fAttackRange = 1.0)이 들어감 -> 합계용은 4개 다 0.0f로 지정 초기화
        //       예: StatBlock tAdd = {.fMaxHp = 0, .fAttackDamage = 0, .fMoveSpeed = 0, .fAttackRange = 0}

    for (const StatModifier& tMod : spMods):     // [C++] span도 범위 for 가능
        if tMod.eOp == ModOp::Add: GetField(tAdd, tMod.eStat) += tMod.fValue
        else:                      GetField(tMul, tMod.eStat) += tMod.fValue

    StatBlock tFinal = tBase              // 복사본 (원본 tBase는 const라 애초에 못 바꿈 = 덮어쓰기 금지가 타입으로 보장)
    for 4가지 StatType 각각 eStat:
        float& fOut = GetField(tFinal, eStat)
        fOut = (fOut + GetField(tAdd, eStat)) * (1 + GetField(tMul, eStat))
        fOut = std::max(0.0f, fOut)       // 결정 3
        // [C++] enum class는 for로 바로 못 돎 -> 4개를 배열에 넣고 돌기:
        //       constexpr StatType kAll[] = {StatType::MaxHp, StatType::AttackDamage, StatType::MoveSpeed, StatType::AttackRange};
        //       for (StatType eStat : kAll) { ... }
    return tFinal
```

### 2-4. 막히면 보는 곳
- `std::span`을 넘기는 쪽: 테스트 `tests/StatsTest_C.cpp` (`std::array`나 `std::vector`를 그대로 넘기면 span으로 바뀜)
- 참조 반환이 헷갈리면: World의 `Slot& tSlot = m_vecSlots[i]`와 같은 원리 (별명)

## 3. AI 작업 (사용자 구현 후)
- `data/ItemDefs_C.h`: 아이템 정의 (회복 1 + 스탯 3~4종). 각 아이템 = modifier 목록 (GDD 8장 `perStack`)
- 아이템 드랍 (방 클리어 보상) · 획득 · 인벤토리(스택) → modifier 목록 만들기 → `Stats::Calculate` → 플레이어 최종 스탯
- 최종 스탯 연결: Motor 이동 속도, Combat 공격력·범위, 최대 HP (최대 HP가 늘면 늘어난 만큼 현재 HP도 +)
- HUD: 먹은 아이템 아이콘 줄
