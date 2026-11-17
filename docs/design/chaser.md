# Chaser 적 + FSM — 설계

> 상태: **합의됨 (2026-11-17)** — 결정 1~5 전부 AI 추천.
> **역할 변경 (사용자 요청):** "World만 관리한 사람이 되기 싫다, 적·플레이어가 실제로 어떻게 도는지 알고 싶다"
> → **EnemyAI(FSM)는 사용자가 구현.** AI는 데이터·공격 공용화·스펙 테스트·Game/Render 연결만.
> 기준: GDD 11장 적 FSM, CLAUDE.md "적 AI는 명시적 FSM (enum class + 상태별 Enter/Update, 타이머 전이)"

## 1. 결정 기록
| # | 질문 | 결정 |
|---|---|---|
| 1 | 적 공격 시간·크기 | 종류별 `AttackDef` → Combat `UpdateAttack(..., const AttackDef&, ...)` 공용 (**완료, AI**) |
| 2 | FSM 상태 저장 | `Entity`에 `EnemyAIState tAI;` |
| 3 | 플레이어 찾기 | `World.Get(hPlayer)` (아는 핸들 조회 = 주변 검색 아님) |
| 4 | 예고 중 맞으면 | 그대로 공격 (끊기는 경직 작업 때) |
| 5 | 예고 표시 | 적 색 주황 깜빡임 (Render, AI) |

## 2. 상태 흐름
```
Idle ──(감지: 가로·세로 둘 다 < fDetectRange)──> Chase ──(붙음: 가로 간격 <= fAttackRangeX, 세로 차 <= fAttackRangeY)──> Windup
 ^                                                │                                                                  │ fWindupTime 끝
 └────────(가로 거리 > fLoseRange, 또는 플레이어 없음)─┘                                                                  v
                                                Chase <──(fRecoverTime 끝)── Recover <──(공격 키 1틱 누르고 바로)── Attack
```
- AI는 **의도만 씀**: `tMoveIntent.iMoveDir`, `tAttackIntent.bAttackPressed`. 실제 이동은 Motor·Physics, 공격 단계·판정은 Combat이 이미 함.
- Dead 상태 없음: HP 0이면 Combat이 Destroy 예약 → 그 틱 AI는 `fHp <= 0`이면 건너뜀 → 다음 Flush에 사라짐.

## 3. 사용자 작업 — 이 장만 따라가면 됨
### 3-1. 만들 파일
| 파일 | 내용 |
|---|---|
| `src/systems/EnemyAI.h` | `EnemyState`, `EnemyAIState`, `class EnemyAI` 선언 |
| `src/systems/EnemyAI.cpp` | `Update` 구현 + 상태별 함수 |
| `src/core/Entity_U.h` | `#include "systems/EnemyAI.h"` + `EnemyAIState tAI;` |

- 파일을 만든 뒤 `cmake -S . -B build` 한 번 (CMake가 `EnemyAI.cpp`를 찾으면 자동 포함, 출력 `EnemyAI 구현 포함: ON`).
- 그러면 스펙 테스트 `tests/EnemyAITest_C.cpp`(9개)도 같이 돎 → **전부 통과하면 완료.** 이름(`EnemyState::Idle`, `tAI.eState`, `EnemyAI::Update`)은 테스트가 쓰므로 그대로.
- 리뷰 때 AI가 `_U` 접미사·명명 정리. Game 연결(호출 1줄)과 Render 예고 표시는 AI가 함.

### 3-2. EnemyAI.h
```
#pragma once
#include "core/Handle_U.h"

enum class EnemyState { Idle, Chase, Windup, Attack, Recover }

struct EnemyAIState {
    EnemyState eState = EnemyState::Idle
    float fTimer = 0.0f                     // 현재 상태 남은 시간 (Windup·Recover만 씀)
    bool bPassive = false                   // true = 튜토리얼 허수아비: AI가 아무것도 안 함 (11/17 추가)
}

class World;                                // [C++] 전방 선언. World_U.h를 include하면 안 됨!
                                            //   Entity_U.h -> EnemyAI.h -> World_U.h -> Entity_U.h 순환이 생김.
                                            //   여기선 World&(참조)만 쓰니까 "이런 클래스가 있다"만 알려 주면 충분
class EnemyAI {
public:
    void Update(World& tWorld, Handle hPlayer, float fDt) const
                                            // [C++] 끝의 const: EnemyAI 자신은 상태가 없음 (상태는 Entity.tAI에)
}
```

### 3-3. EnemyAI.cpp
```
#include "systems/EnemyAI.h"
#include "core/World_U.h"                   // 여기서는 include OK (cpp는 순환과 상관없음)
#include "data/EnemyDef_C.h"                // kChaserDef: 거리·시간 값
#include <cmath>                            // std::abs

// [C++] 이 파일에서만 쓰는 함수는 namespace { } 안에 (C#의 private static 도우미와 비슷)
namespace {
    Enter(EnemyAIState& tAI, EnemyState eNext) -> void:
        tAI.eState = eNext
        tAI.fTimer = eNext가 Windup이면 kChaserDef.fWindupTime
                     Recover면 kChaserDef.fRecoverTime
                     나머지 0
        // [C++] switch나 if/else 둘 다 OK. enum class 값은 EnemyState::Windup처럼 이름까지 붙여야 함

    CenterX(const Body& tBody) -> float:   return vPos.fX + vSize.fX * 0.5
    CenterY(const Body& tBody) -> float:   return vPos.fY + vSize.fY * 0.5

    UpdateEnemy(Entity& tEnemy, const Entity* pPlayer, float fDt) -> void:
        EnemyAIState& tAI = tEnemy.tAI      // [C++] & 꼭 붙이기. 없으면 복사본을 고치게 됨 (World Flush 때와 같은 함정)
        tAI.fTimer -= fDt

        // 플레이어가 없으면(무효 핸들) 쫓을 대상이 없음
        if pPlayer == nullptr:
            if tAI.eState != Idle: Enter(tAI, Idle)
            return

        fDx = CenterX(플레이어 몸) - CenterX(적 몸)                // + 면 플레이어가 오른쪽
        fDy = std::abs(CenterY(플레이어 몸) - CenterY(적 몸))
        fGapX = std::abs(fDx) - (플레이어 폭 + 적 폭) * 0.5       // 몸 끝 사이 간격 (겹치면 음수)

        switch tAI.eState:
            case Idle:
                if std::abs(fDx) < kChaserDef.fDetectRange and fDy < kChaserDef.fDetectRange: Enter(tAI, Chase)
                break
            case Chase:
                if std::abs(fDx) > kChaserDef.fLoseRange: Enter(tAI, Idle); break
                tEnemy.tMoveIntent.iMoveDir = (fDx > 0) ? 1 : -1  // Motor가 이걸 보고 걷고 facing도 맞춤
                if fGapX <= kChaserDef.fAttackRangeX and fDy <= kChaserDef.fAttackRangeY: Enter(tAI, Windup)
                break
            case Windup:
                // 이동 의도 0 -> Motor가 감속해서 멈춤. 그냥 기다림
                if tAI.fTimer <= 0: Enter(tAI, Attack)
                break
            case Attack:
                tEnemy.tAttackIntent.bAttackPressed = true         // 딱 이 1틱. 나머지는 Combat이 진행
                Enter(tAI, Recover)
                break
            case Recover:
                if tAI.fTimer <= 0: Enter(tAI, Chase)
                break
        // [C++] case 끝에 break 빠뜨리면 다음 case로 그대로 내려감 (C#은 컴파일 에러로 막아 주지만 C++은 안 막음)
}

EnemyAI::Update(World& tWorld, Handle hPlayer, float fDt) const:
                                            // [C++] cpp에서는 함수 이름 앞에 "EnemyAI::"를 붙여 클래스 소속을 표시
    const Entity* pPlayer = tWorld.Get(hPlayer)     // 읽기만 할 거라 const 포인터. 무효면 nullptr

    tWorld.ForEachAlive( [&](Handle, Entity& tEntity) {
                                            // [C++] 람다 = C#의 (h, e) => { ... }
                                            //   [&] = 바깥 변수(pPlayer, fDt)를 참조로 가져다 씀
                                            //   Handle은 안 써서 이름 생략 (이름 붙이고 안 쓰면 /W4 경고)
        if tEntity.eType != EntityType::Enemy: return       // [C++] 람다 안에선 continue 대신 return
        if tEntity.fHp <= 0: return                         // 삭제 예약 중
        if tEntity.tAI.bPassive: return                     // 허수아비 (의도도 안 건드림)
        tEntity.tMoveIntent = {}                            // 매 틱 의도를 새로 씀 ({} = 전부 기본값)
        tEntity.tAttackIntent = {}
        UpdateEnemy(tEntity, pPlayer, fDt)
    })
```

### 3-4. 막히면 보는 곳
- 의도가 실제로 어떻게 움직임이 되는지: `src/systems/PlayerMotor_C.cpp` `Update` (iMoveDir → 속도, facing)
- 공격 키가 어떻게 히트박스가 되는지: `src/systems/Combat_C.cpp` `UpdateAttack` → `ResolveHits`
- 한 틱에 누가 먼저 도는지: `docs/code-guide.md` 4장, `src/core/Game_C.cpp` `FixedUpdate`

## 4. AI 작업
- (완료) `data/AttackDef_C.h`, `PlayerDef.tAttack`, `EnemyDef` AI 수치, `Combat::UpdateAttack` 공용화, 스펙 테스트 8개, CMake 자동 포함
- (사용자 구현 후) `Game.FixedUpdate` 맨 앞에 `m_tEnemyAI.Update(m_tWorld, m_hPlayer, fDt)`, Render Windup 깜빡임, 리뷰·명명 정리
- 플레이어가 맞는 경로는 이미 있음 (`IsHostile(Enemy, Player)` → HP·넉백·깜빡임·파편). 새 코드 없음
- 플레이어 HP 0: 지금은 아무 일 없음 → 다음 단계 RoomFlow가 게임 오버
