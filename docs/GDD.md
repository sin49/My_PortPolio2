# 기획서 v0.1 — C++ 2D 룸 클리어 로그라이트 (가칭: Main)

> 졸업작품 NecroSoul(Unity)의 핵심 루프를 C++ + raylib로 다시 설계하는 프로젝트.
> 목표는 "플레이 가능한 프로토타입"이고, 포폴 포인트는 **책임 분리 설계**와 **C++ 수명 관리**임.

## 1. 한 줄 요약
근접 공격 캐릭터가 방에 들어가서 적을 전멸시키면 아이템을 얻고 다음 방으로 넘어감. 아이템은 겹쳐 먹을수록 강해짐. 3스테이지를 클리어하면 승리.

## 2. 포폴에서 보여줄 것
| 포인트 | 내용 | 근거가 되는 과거 문제 |
|---|---|---|
| **책임 분리** | 시스템별로 데이터 소유와 수정 권한을 나눔. 중심 클래스(GameManager류) 금지 | NecroSoul `PlayerCharacter` 1,139줄, `Gamemanager`·Unity Fighter `Stage`에 책임 집중 |
| **스탯 재계산** | 아이템·버프는 modifier 목록으로 두고 매 프레임 기본값부터 다시 계산 | NecroSoul `ItemEffect0.effect/uneffect`, Unity Fighter `BuffDeactive`가 값을 덮어써서 중첩 시 꼬임 |
| **C++ 수명 관리** | 소유권(`unique_ptr`/값), 엔티티는 핸들로 참조, 순회 중 삭제 금지 | Unity Fighter 순회 중 `Remove(this)`, 이벤트 해제 누락 |
| **데이터 분리** | 아이템·적·방 정의를 코드 밖 데이터(테이블/텍스트)로 | `switch(Foreignkey)` + case 5~10 매직넘버, `room_element == 1~4` |
| (확장) **절차적 맵** | 층 단위 방 그래프 생성 + 도달 가능성 검증 | NecroSoul V1의 "끊긴 방" 문제를 알고리즘 검증으로 해결 |
| (확장) **A\*** | 비행 적 경로 탐색 | NecroSoul `Astar_E_07` 재구현 |

## 3. 게임 규칙 (MVP)
- 2D 횡스크롤 액션. 화면 하나 = 방 하나 (카메라 스크롤 없음).
- 화면 규격: 창 1280x720, 타일 32px, 방 = 40x22칸(1280x704). 남는 위·아래 8px는 검은 프레임. (11/16 결정)
- 점프 보조: 코요테 타임 + 점프 버퍼 + 가변 점프. 상세는 `docs/design/player-movement.md`.
- 플레이어: 이동(관성 있음), 점프, **대시(누르는 동안 빨리 달리기, 게이지 소모)**, **근접 기본 공격**. (대시는 11/17 추가)
- 방에 들어가면 문이 잠기고 적이 등장함. 전멸하면 문이 열리고 **아이템 1개 드랍**.
- 스테이지 1개 = 방 3개 일렬. 스테이지가 오를수록 적 수와 체력 증가.
- 3스테이지 마지막 방 클리어 = 승리. 플레이어 HP 0 = 게임 오버 → 처음부터.

## 4. 조작
| 키 | 동작 |
|---|---|
| ← / → | 이동 (11/17 A/D에서 변경) |
| Z | 점프 (길게 누르면 높이) |
| X | 공격 |
| Shift | 대시 (누르고 있는 동안 빨리 달리기) |
| R | (게임 오버 화면) 재시작 |

## 5. 콘텐츠 수량 (MVP)
- **플레이어** 1종. 공격 1종(콤보 없음).
- **적** 2종
  - Chaser: 플레이어 쪽으로 걸어와서 근접 공격. (MVP 필수)
  - Shooter: 거리 유지하며 투사체 발사. (4일차, 시간 없으면 제외)
- **방 레이아웃** 3개를 텍스트 타일맵으로 만들고 재사용.
- **아이템** 5종, 전부 스택형.
  - 예: 공격력 +, 공격 속도 +, 이동 속도 +, 최대 HP +, 공격 범위 +

## 6. 근접 공격 판정 (원거리보다 단기 구현이 어렵지 않은 이유)
```
// 공격 입력 시
attack.active = true; attack.timer = 0.15s;
hitbox = 플레이어 앞쪽 사각형 (크기 = 기본 범위 * 범위 modifier)
attack.alreadyHit.clear()     // 한 번 휘두를 때 같은 적을 두 번 때리지 않게

// 매 프레임 (active 동안)
for enemy in 적 목록:
    if overlap(hitbox, enemy.box) and enemy.id not in alreadyHit:
        DamageEvent 발행; alreadyHit.add(enemy.id)
```
- 투사체 풀·이동·수명 관리가 필요 없어서 **코드량은 원거리보다 적음**.
- 대신 "손맛"(경직, 넉백, 적 공격 예고)이 신경 쓰이는데, MVP에선 넉백 + 피격 깜빡임만 넣음.

## 7. 구조 설계 (책임 분리)
**원칙: 데이터는 한 곳이 소유하고, 수정은 정해진 시스템만 함. 다른 시스템은 읽거나 이벤트를 보냄.**

| 모듈 | 책임 | 하면 안 되는 것 |
|---|---|---|
| `Game` | 메인 루프, 시스템 호출 순서 | 게임 규칙 로직 |
| `World` | 엔티티(플레이어·적·드랍) 저장, 생성/삭제 예약 | 판정·AI |
| `Input` | 키 입력 → 의도(`MoveDir`, `JumpPressed`, `AttackPressed`) | 엔티티 직접 수정 |
| `PlayerMotor` | 플레이어 의도 → 속도 (코요테·점프 버퍼·가변 점프 타이머) | 위치 직접 수정, 충돌 판정 |
| `Physics` | 중력, 속도 적용, 타일 충돌, 접지 판정 (플레이어·적 공용) | 데미지, 입력 의도 해석 |
| `Combat` | 히트박스 겹침 → HP 차감, 사망 표시, 결과(`HitReport`) 기록. `DamageEvent` 발행은 Events 도입 때(수·목) | 아이템 효과 직접 적용, 연출 |
| `EnemyAI` | 적 의도 결정(이동 방향, 공격 시작) | 위치 직접 수정 (의도만) |
| `Stats` | 기본값 + modifier → 최종 스탯 계산 | 값 덮어쓰기 |
| `Items` | 아이템 획득/스택 관리, modifier 제공 | 스탯 직접 수정 |
| `RoomFlow` | 방 상태(전투 중/클리어/다음 방), 스테이지 진행, 승패 | 렌더링 |
| `Render` | 화면 그리기 | **게임 상태 수정 금지** (읽기 전용) |
| `Events` | 시스템 간 알림(`OnDamage`, `OnKill`, `OnRoomClear`) | — |

### 한 프레임 순서
```
Input.Update()            -> 의도 기록
EnemyAI.Update()          -> 적 의도 기록
Stats.Recalculate()       -> 최종 스탯 갱신
PlayerMotor.Update(dt)    -> 의도 -> 속도 (점프 보조)
Physics.Update(dt)        -> 중력/이동/충돌
Combat.Update(dt)         -> 판정, DamageEvent
Events.Dispatch()         -> OnKill, OnDamage 등 처리
RoomFlow.Update()         -> 클리어/전환/승패
World.FlushRemovals()     -> 삭제 예약된 엔티티 실제 제거 (순회 중 삭제 금지)
Render.Draw()
```

## 8. 아이템·스탯 설계
```
struct StatBlock { float maxHp, atk, atkSpeed, moveSpeed, attackRange; }

struct Modifier { StatType stat; ModOp op(Add|Mul); float value; }

ItemDef { id, name, vector<Modifier> perStack }   // 데이터 테이블

// 매 프레임
final = base
for (item, stack) in inventory:
    for mod in item.perStack:
        apply(final, mod, stack)     // Add 먼저 전부, Mul 나중에 전부
```
- 아이템을 잃거나 버프가 끝나면 **목록에서 빼기만** 하면 됨. 되돌리기 코드가 없음.
- TODO(논의): NecroSoul처럼 스택 3/5개 도달 시 특수효과(단계 효과)를 넣을지 → MVP 이후.

## 9. 일정 (코테·지원 일정 반영)
| 날짜 | 개발 가용 | 목표 |
|---|---|---|
| 11/16 월 (밤) | 1시간 | 환경 세팅: CMake + raylib 창 띄우기, git init, CLAUDE.md 커밋 |
| 11/17 화 | 반나절 | 플레이어 이동/점프/타일 충돌, 근접 공격, Chaser 1종, HP |
| 11/18 수 | 반나절 | 방 진행(잠금→전멸→문 열림→다음 방), 3스테이지, 게임 오버/승리 → **MVP 완성** |
| 11/19 목 | 짧게 (에세이) | 아이템 5종 + 스탯 재계산, 아이템 HUD |
| 11/20 금 | 반나절 | 버그·밸런스, (여유 시) Shooter, README, 플레이 영상 → **v0.1 태그 = 제출용 스냅샷** |
| 11/25~ | — | 확장 (아래) |

- **수요일 밤 MVP가 마지노선.** 그때까지 방 진행이 안 되면 아이템은 2종으로 줄임.
- **(11/17 화 사용자 결정) 개발 마감 = 11/18 수요일 밤.** 목~토는 SSAFY 에세이·코테, 이후 바로 포폴 정리 → 펄어비스(C++ 역량) · 넷마블(AI 활용) 지원.
  위 목~금 표는 참고용. 목표: **튜토리얼 → 방 진행 → 간단한 아이템 → 간단한 보스 → 승리/게임 오버**까지 시작부터 끝 흐름. 못 한 것은 "확장 예정".
- **포폴 핵심(노른자)은 사용자가 직접 구현**: World(완료), EnemyAI FSM, Stats, Spatial Hash. 나머지(보스·흐름·UI·사운드·연결)는 AI.
- 적 3종(Chaser 근접 / Shooter 원거리 / 보스), 원거리 공격은 투사체 1종으로 통일. 아이템 = 회복 + 스탯 보정 몇 개.
  UI는 글자 최소화. 사운드는 무료 배경음 + 효과음. 우선순위: 시스템·렌더링 → 튜토리얼 → UI → 사운드. 화·수 전체를 MVP 구현에.
  세부: `docs/design/room-flow-ui.md`, `boss-projectile.md`

## 10. 범위 밖 (확장 후보, 우선순위 순)
1. 층 그래프 맵 생성 + 도달 가능성 검증 (NecroSoul V3 재설계)
2. Shooter 적 + 투사체 풀
3. 아이템 단계 효과 / 이벤트 훅(`OnHit`, `OnKill`) 기반 특수 아이템
4. A\* 비행 적
5. 보스, 상점, 웹 빌드(Emscripten)
6. (사용자 희망, 11/17) **PvP**: 로컬 2인(키 분할) → 네트워크(락스텝/롤백). 고정 타임스텝 + 의도 분리 + 핸들 구조가 전제
7. (사용자 희망, 11/17) **그래픽**: Blasphemous풍 다크 도트 + 서브컬처 캐릭터. MVP는 사각형 유지, Render만 스프라이트 시트로 교체. 제작: PixelLab 생성 + 프레임 손 보정 / 사용자 레퍼런스 스케치

## 11. 기술 적용 계획
### 이번 주 (MVP에 포함, 별도 일정 없음)
| 기술 | 적용 위치 | 어필 |
|---|---|---|
| 세대 번호 핸들 + 고정 슬롯 풀 | `World` 엔티티 저장 | 삭제된 객체 참조를 구조적으로 차단, 할당 없는 재사용 |
| 고정 타임스텝 (1/60s 누적기) | `Game` 메인 루프 | 프레임 독립 물리, Unity `FixedUpdate` 직접 구현 |
| doctest 로직 테스트 | `tests/` (Stats, 핸들 풀) | 렌더 없이 로직 테스트 가능 = 책임 분리 증거 |
| 적 FSM | `EnemyAI` (Idle → Chase → Windup → Attack → Recover, Dead) | 상태별 진입/갱신/종료 분리, 타이머 기반 전이 |
| 공간 질의 인터페이스 | `SpatialQuery::QueryNearby(rect)` | MVP는 전체 순회 구현. Combat/AI는 구현을 모름 |

### 금요일 (수요일 MVP 완료 시)
| 기술 | 내용 |
|---|---|
| Spatial Hashing | `SpatialQuery` 뒤에 균일 격자 해시 구현 추가. 매 고정 틱 재구축 |
| 스트레스 모드 | 디버그 키로 더미 적 500~1000 생성, F키로 순회/해시 전환 |
| 비교 오버레이 | 쌍 비교 횟수, 질의 시간(ms)을 화면 표시 → README에 캡처 |

```
// 적 FSM 의사코드
enum class EnemyState { Idle, Chase, Windup, Attack, Recover, Dead }
Update(e):
  switch e.state:
    Idle:    if 플레이어 감지 -> Enter(Chase)
    Chase:   의도 = 플레이어 방향; if 거리 < 공격거리 -> Enter(Windup)
    Windup:  (예고 깜빡임) timer 끝 -> Enter(Attack)
    Attack:  히트박스 1회 생성 -> Enter(Recover)
    Recover: timer 끝 -> Enter(Chase)
Enter(e, s): e.state = s; e.timer = 상태별 시간(데이터 테이블)
```

```
// 핸들 풀 의사코드
struct Handle { uint32 index; uint32 gen; }
Slot { T value; uint32 gen; bool alive; }

Create(): idx = freeList.pop(); slots[idx].alive = true; return {idx, slots[idx].gen}
Get(h):   s = slots[h.index]; return (s.alive && s.gen == h.gen) ? &s.value : nullptr
Destroy(h): 예약만 함 -> FlushRemovals()에서 alive=false, gen++, freeList.push(idx)
```

### 확장 (11/25 이후, 추천순)
0. (금요일에 못 했다면) Spatial Hashing + 스트레스 비교 — 최우선
1. ImGui 디버그 패널 + 데이터 테이블 핫리로드 (CSV 에디터 경험의 연장)
2. 층 그래프 맵 생성 + BFS 도달 가능성 검증 + 시드 재현
3. 이벤트 훅 아이템 + RAII 구독 토큰 (소멸 시 자동 해제)
4. A\* 비행 적
5. Data-Oriented 배치 (스트레스 모드에서 측정 가능할 때)

### 하지 않음
멀티스레드(쓸 곳 없음), 커스텀 할당자(측정 불가·풀로 충분), 풀 ECS(적응 비용 과다)

## 12. 열린 논의
- TODO(논의): 방 레이아웃 데이터 포맷 (텍스트 그리드 vs 간단한 JSON)
- TODO(논의, MVP 이후): 맵을 화면보다 크게 만들 경우 카메라 처리 (지금은 화면 = 방). 플레이어 크기 24x40은 유지하고 맵을 키우는 방향.
- TODO(논의): 적 생성 수를 스테이지별 고정값으로 할지, 공식(`base + stage * k`)으로 할지
- TODO(논의): 아이템 드랍을 랜덤 1개로 할지, 3개 중 1개 선택으로 할지 (선택형은 UI 비용 있음)
