# 플레이어를 World로 옮기기 — 설계

> 상태: **합의됨 (2026-11-17)** — 1-A, 2-A(사용자가 Entity 필드 추가), 3 그대로. 사용자 이유: 나중에 PvP를 만들면 A가 밑작업이 됨. 기준: `world.md` 결정 2 ("플레이어도 World에, Game은 핸들만")
> 목적: 적(Chaser)도 플레이어와 **같은 시스템**(PlayerMotor·Physics·Combat)을 쓰게 하기 위한 사전 작업. 게임 동작은 바뀌지 않아야 함.

## 1. 결정할 것
| # | 질문 | 선택지 | AI 추천 |
|---|---|---|---|
| 1 | 플레이어의 의도·이동 상태·공격 상태를 어디에 둘까 | **A.** `Entity` 필드로 (`tMoveIntent`, `tAttackIntent`, `tMotor`, `tAttack`) / **B.** 플레이어 전용이니 `Game`에 남기고 World엔 Body·HP만 | **A.** Chaser도 "의도 → 이동 → 물리 → 공격"을 똑같이 탐. AI가 `tMoveIntent`를 쓰면 같은 `PlayerMotor`가 움직여 줌 (결정 1 "Entity 하나에 다 넣음"과 일치). B면 적용 상태를 또 따로 만들어야 함 |
| 2 | `Entity_U.h`에 필드 추가는 누가 | **A.** 사용자가 4줄 추가 (아래 2장 목록) / **B.** AI가 추가 → 파일이 섞이므로 `Entity.h` + `// work:` 주석 | **A.** 4줄이라 금방이고 파일이 `_U`로 남음 |
| 3 | 이름 `PlayerMotor`, `PlayerMotorState` | 그대로 / `Motor`, `MotorState`로 변경 | **지금은 그대로.** Chaser를 붙여 보고 정말 공용이 되면 그때 이름 변경 (기계적 작업) |

## 2. Entity에 추가할 필드 (결정 1-A, 2-A 기준)
```cpp
// src/core/Entity_U.h 에 include 3개 + 필드 4개
#include "core/AttackIntent_C.h"
#include "core/MoveIntent_C.h"
#include "systems/Combat_C.h"        // AttackState
#include "systems/PlayerMotor_C.h"   // PlayerMotorState

struct Entity {
    ... 기존 3개 ...
    MoveIntent tMoveIntent;          // Input(플레이어) 또는 AI(적)가 기록
    AttackIntent tAttackIntent;
    PlayerMotorState tMotor;         // 점프 보조·대시 게이지·facing
    AttackState tAttack;             // 공격 단계·히트박스
};
```
- `core`가 `systems` 헤더를 include하는 게 걸리면: 상태 구조체(`PlayerMotorState`, `AttackState`)만 `core/`로 옮기는 방법도 있음 → AI가 구현할 때 기계적으로 처리 가능. **우선은 위 방식.**

## 3. 의사코드 (AI 구현)
```
Game 멤버:
    World m_tWorld
    Handle m_hPlayer                         // 플레이어는 핸들로만 보관 (포인터 X)
    (삭제) m_tPlayerBody, m_tPlayerMotorState, m_tPlayerAttack, m_tPlayer~Intent

Game(room):
    m_hPlayer = m_tWorld.Create(Entity{ eType = Player, tBody = {spawn, kPlayerDef.vSize} })

Run() 매 렌더 프레임:
    pPlayer = m_tWorld.Get(m_hPlayer)       // 매번 다시 꺼냄 (오래 들고 있지 않음)
    if pPlayer: m_tInput.Poll(pPlayer->tMoveIntent, pPlayer->tAttackIntent)
    for 고정 틱:
        FixedUpdate(kStep)
        pPlayer = Get(m_hPlayer); if pPlayer: 래치 해제 (bJumpPressed, bAttackPressed = false)
    Render.Draw(room, const World&, m_hPlayer, debug)

FixedUpdate(dt):
    ForEachAlive(h, e): PlayerMotor.Update(e.tMoveIntent, e.tBody, e.tMotor, 속도(e), dt)
    ForEachAlive(h, e): Physics.Update(e.tBody, room, dt)
    ForEachAlive(h, e): Combat.UpdatePlayerAttack(e.tAttackIntent, e.tBody, e.tMotor.iFacing, e.tAttack, dt)
    m_tWorld.FlushRemovals()                // 틱 끝에서 일괄 삭제 (CLAUDE.md: 순회 중 삭제 금지)
    // 속도(e): 지금은 플레이어뿐이라 kPlayerDef.fMoveSpeed. Chaser 때 종류별 Def에서 꺼냄

Render.Draw(room, const World& tWorld, Handle hPlayer, debug):
    pPlayer = tWorld.Get(hPlayer)           // const Get 사용 (World를 못 바꿈 = Render 읽기 전용 규칙)
    if pPlayer: 지금처럼 플레이어·히트박스 그리기
```
- 시스템 함수(`PlayerMotor`, `Physics`, `Combat`)는 **안 바뀜**. 원래 `Body&`/상태`&`만 받게 만들어 둔 덕분 → 기존 테스트 그대로 통과해야 함.
- 확인: 실행해서 조작감이 어제와 똑같으면 성공.

## 4. 나중에 사용자에게 요청할 것 (지금 아님)
- 적을 화면에 그릴 때 Render가 `const World&`로 순회해야 함 → `World_U.h`에 **const 버전 `ForEachAlive`** 필요
  (`fnVisit(Handle, const Entity&)`). 지금 있는 것과 거의 같음. Chaser 때 의사코드 줌.
