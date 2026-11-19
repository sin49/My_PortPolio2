# 아이템 (드랍·획득·스택 → Stats 연결) — 설계

> 상태: **합의됨 (2026-11-19)** — 결정 전부 AI 추천 + 사용자 수정 3개 (아래 ★). AI 구현. 사용자 Stats(`Stats_U`)를 실제 게임에 연결하는 단계.
>
> ★ 사용자 수정 (11/19):
> 1. **회복약은 방 클리어 보상에서 빠짐** → 대신 **적이 죽을 때마다 확률로** 떨어짐 (다른 4종은 클리어 보상 그대로)
> 2. **회복약 확률 = 플레이어 HP에 따라**: HP 가득 = 0%, 그 외 `5% + 35% × (잃은 HP 비율)` (HP 50% → 22.5%, HP 10% → 36.5%)
> 3. **인벤토리 UI = 텍스트**, HP·대시 게이지 바로 밑. 회복약은 인벤토리에 안 남음
>    - raylib 기본 글꼴은 한글이 없음 → 화면 글자는 영어 (`Feather x2  SPD +24%`). 한글 글꼴은 확장 예정
> 기준: GDD 8장 (ItemDef = perStack modifier 목록), stats.md 3장, CLAUDE.md "스탯은 덮어쓰지 않음"

## 1. 결정 (AI 추천, 사용자 확인 필요)
| # | 질문 | AI 추천 | 이유 |
|---|---|---|---|
| 1 | 드랍 방식 (GDD 12장 TODO) | **방 클리어 시 랜덤 1개** (3개 중 선택은 확장 예정) | 선택 UI 비용 없음. 마감 우선 |
| 2 | 어디에 떨어지나 | **마지막 적이 죽은 자리**에서 위로 살짝 튀며 떨어짐 (중력 O) | 클리어 순간 시선이 거기 있음 |
| 3 | 줍기 | 몸이 닿으면 자동 획득 (키 없음). 안 줍고 문으로 가도 됨 | 조작 추가 없음 |
| 4 | 종류 (GDD 5종) | 아래 표 | 회복 1 + 스탯 4 |
| 5 | 인벤토리 위치 | `Items` 클래스(Game 소유)가 **아이템별 스택 수**만 들고 있음. Entity는 안 바꿈 | 플레이어 1명. Drop 엔티티가 무슨 아이템인지는 `Items`가 **핸들 목록**으로 기억 |
| 6 | 최대 HP 증가 시 | 늘어난 만큼 현재 HP도 + (회복 아이템은 최대 HP에서 멈춤) | 먹자마자 체감 |
| 7 | 재계산 시점 | **매 틱** `Stats::Calculate(기본값, modifier 목록)` (GDD 8장 그대로) | 4개 스탯 × 몇 개 modifier = 비용 0에 가까움. 캐시 무효화 버그 없음 |

| 아이템 | 효과 (스택 1개당) | 색 |
|---|---|---|
| 회복약 `Potion` | 즉시 HP +30 (스택 안 됨, 인벤토리에 안 남음) | 빨강 |
| 심장 `Heart` | 최대 HP **Add +20** | 분홍 |
| 숫돌 `Whetstone` | 공격력 **Add +5** | 주황 |
| 깃털 `Feather` | 이동 속도 **Mul +0.12** (+12%) | 하늘 |
| 긴 칼날 `LongBlade` | 공격 범위 **Mul +0.25** (히트박스 가로·세로 ×1.25) | 노랑 |

## 2. 데이터 (AI) — `data/ItemDefs_C.h`
```
enum class ItemId { Potion, Heart, Whetstone, Feather, LongBlade, Count }
struct ItemDef { svName, svDesc, ItemColor(r,g,b), float fHealOnPickup, std::span<const StatModifier> spPerStack }
kItemDefs[Count] = { ... 위 표 ... }
GetItemDef(ItemId) -> const ItemDef&
kPlayerBaseStats = StatBlock{ kPlayerDef.fMaxHp, kPlayerDef.fAttackDamage, kPlayerDef.fMoveSpeed, 1.0 }
```
- 데이터 = 테이블. "아이템 3번이면 공격력 +5" 같은 switch·매직넘버 없음 (GDD 2장 문제 1번).

## 3. 의사코드 (AI) — `systems/Items_C.h/.cpp` (raylib 비의존 → 테스트 가능)
```
class Items:
    std::array<int, Count> m_arrStacks = {}            // 아이템별 개수
    std::vector<DropEntry{Handle h, ItemId e}> m_vecDrops   // 바닥에 떨어진 것
    std::uint32_t m_uRng                                // 랜덤 1개 고르기 (시드 고정 가능 → 테스트)

    Reset():  스택 0, 드랍 목록 비움                    // 새 게임
    SpawnDrop(World&, vFeet, ItemId) -> Handle:         // Drop 엔티티 (12x12, 위로 튐, 중력 O)
        h = world.Create(Entity{ eType = Drop, tBody{...}, fHp = 1 })  -> m_vecDrops에 {h, id}
    SpawnRandomDrop(World&, vFeet): id = 랜덤(0..Count-1) -> SpawnDrop
    Update(World&, Handle hPlayer, const StatBlock& tFinal, vecOutPickups):
        m_vecDrops에서 world.Get(h) == nullptr 인 것 제거   // 방이 바뀌어 지워진 드랍 = 세대 핸들이 알아서 무효
        pPlayer 없으면 return
        각 드랍: 플레이어 몸과 겹치면
            회복약: player.fHp = min(fHp + fHeal, tFinal.fMaxHp)
            그 외: m_arrStacks[id] += 1
            world.Destroy(h), 목록에서 제거, vecOutPickups에 {id, 위치} (HUD 알림·파편)
        // [순회 중 삭제 금지] 목록은 지울 인덱스를 모은 뒤 뒤에서부터 지움 (또는 std::erase_if)
    BuildModifiers(vecOut):                             // Stats에 넘길 목록
        vecOut.clear()
        각 아이템 id, 스택 n: perStack을 n번 push      // 스택 = 같은 modifier 여러 번 (Stats 스펙 테스트 5번과 같은 규칙)
    GetStack(id) const -> int
```

## 4. Game 연결 (AI)
```
Game 멤버: Items m_tItems; StatBlock m_tPlayerStats = kPlayerBaseStats; vector<StatModifier> m_vecMods;

FixedUpdate 맨 앞 (RoomFlow 다음):
    m_tItems.BuildModifiers(m_vecMods)
    StatBlock tNew = Stats::Calculate(kPlayerBaseStats, m_vecMods)      // 원본 kPlayerBaseStats는 constexpr = 못 바꿈
    if tNew.fMaxHp > m_tPlayerStats.fMaxHp: player.fHp += 차이            // 결정 6
    m_tPlayerStats = tNew
쓰는 곳:
    Motor 속도:     플레이어 = m_tPlayerStats.fMoveSpeed (지금 kPlayerDef.fMoveSpeed 자리)
    공격 히트박스:  플레이어 AttackDef 복사본, vSize *= fAttackRange
    공격력:         Combat.ResolveHits(..., float fPlayerDamage) 인자 추가 (지금 kPlayerDef.fAttackDamage 자리)
    HUD·Render:     최대 HP = m_tPlayerStats.fMaxHp (FrameView에 const StatBlock& 추가)
드랍 시점: RoomFlow가 Locked -> Cleared 되는 틱 → 마지막으로 죽은 적 위치(HitReport.bKilled의 vPos)에 SpawnRandomDrop
    // RoomFlow는 아이템을 모름. Game이 "이번 틱에 클리어됐나"(RoomFlow.WasClearedThisTick())만 보고 Items를 부름
새 게임: m_tItems.Reset()
Physics: Drop도 중력으로 떨어짐 (Motor는 건너뜀, 지금 Game 루프에 종류 검사 있음)
```

## 5. 화면 (AI)
- 드랍: 아이템 색 마름모, 위아래로 둥실 + 위에 이름 작게
- 획득: 화면 가운데 위 "깃털 — 이동 속도 +12%" 1.5초 표시 + 아이템 색 파편
- HUD: HP 바 아래 아이템 칸 줄 (색 네모 + `x2`)
- HP 바 길이는 최대 HP 기준 그대로 (값만 `현재/최대` 숫자로)

## 6. 테스트 (AI) — `tests/ItemsTest_C.cpp`
- 줍기: 겹치면 스택 +1, 드랍 엔티티 삭제 / 안 겹치면 그대로
- 회복약: HP +30, 최대 HP 넘지 않음, 스택 안 늘어남
- BuildModifiers: 깃털 2개 → modifier 2개 → `Stats::Calculate` 이동 속도 = 340 × 1.24
- 방이 바뀌어 드랍 엔티티가 지워지면 목록에서도 빠짐 (무효 핸들 = nullptr)
- 같은 시드면 같은 랜덤 드랍 (재현 가능)
