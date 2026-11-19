# 방 진행 · 튜토리얼 · UI — 설계 (AI 구현)

> 상태: **1차 구현 (2026-11-17)** — 방 4개(튜토리얼 70칸 / 방1 56칸 / 방2 64x26 (11/19 2차 축소 + 발판 계단·위쪽 적) / 보스 50칸), 카메라, 문, 타이틀·게임오버·승리, HUD.
> 사용자 추가 요청 반영: 적 크기 3종(1·2·3대, 작은 적이 대부분), 남은 적 칸 표시, **남은 적이 처음의 1/4 이하면 화면 밖 적 방향 화살표**.
> 남은 것: 허수아비 bPassive(사용자 EnemyAI 후), Shooter·보스 실제 생성(지금은 Chaser로 대체), 아이템, 사운드.
> 결정 1~6 전부 AI 추천. 추가 사용자 결정:
> - 적 3종: Chaser(근접) / **Shooter(원거리, 단일 투사체)** / 보스. **원거리 공격은 투사체 1종류로 통일** → 풀링 단순화
> - 아이템: 회복 + 간단한 스탯 보정 몇 개
> - **UI는 글자 최소화** (바·아이콘·색 위주, 글자는 꼭 필요한 것만)
> - 사운드: 배경음 + 간단한 효과음. 무료(CC0 등 라이선스 확인된 것). raylib은 MIDI 재생 불가 → OGG/WAV
> - 우선순위: **시스템·렌더링 → 튜토리얼 → UI → 사운드**. 사소한 것은 주말 포폴 작업 때
> 합의 전: 제안 (2026-11-17). 사용자 위임: "UI도 AI가 대충". 기준: GDD 3장(흐름), 7장(RoomFlow 책임: 방 상태·진행·승패, 렌더링 X)
> 목표: **타이틀 → 튜토리얼 → 방 1 → 방 2 → 보스 방 → 승리**, 중간에 HP 0이면 게임 오버 → 재시작.

## 1. 결정할 것
| # | 질문 | AI 추천 | 이유 |
|---|---|---|---|
| 1 | 방 데이터 위치 | **코드 상수** (`data/Rooms_C.h`, 지금 TestRoom처럼) | 파일 로드·에러 처리 시간 절약. `assets/rooms/*.txt` 로드는 확장 예정 |
| 2 | 다음 방으로 갈 때 | 플레이어 제외 엔티티 전부 Destroy → Flush → 새 방 적 생성. **플레이어 HP·아이템 유지**, 위치만 스폰으로 | 로그라이트 기본. 핸들은 그대로라 `m_hPlayer` 안 바뀜 |
| 3 | 문 | 타일이 아니라 **방 오른쪽 끝 사각형 트리거**. 잠김 = 빨강 / 열림 = 초록, 열린 문에 닿으면 다음 방 | Tilemap·Physics 수정 없음 |
| 4 | 화면 글자 | **영어** | raylib 기본 폰트에 한글 없음. 한글 폰트 로드는 확장 예정 |
| 5 | 재시작 | 게임 전체 초기화 (World 비우고 플레이어 새로 생성, 튜토리얼부터) | 가장 단순·버그 적음 |
| 6 | 튜토리얼 | 별도 시스템 없이 **방 데이터에 안내 문구 몇 줄** + 허수아비 1마리 (AI 없음) | 시간 대비 효과. 단계별 체크(점프했나 등)는 확장 예정 |

## 2. 데이터 (AI)
```
// data/Rooms_C.h
RoomDef {
    rows[22]                     // '#' 벽 / '.' 빈칸
    vPlayerSpawn
    span<const EnemySpawn>       // { vPos, eType (Enemy / Boss), bDummy(튜토리얼 허수아비: AI 안 붙음) }
    rcDoor                       // 오른쪽 끝 트리거
    span<const string_view> hints   // 튜토리얼 안내 (화면 위쪽에 표시)
    svName                       // "Tutorial", "Room 1", ...
}
kRooms = { Tutorial, Room1(Chaser 3), Room2(Chaser 5, 발판 많음), BossRoom(Boss 1, 문 없음) }
```
- 허수아비(bDummy): EnemyAI가 건너뛰게 `EnemyAIState`에 표시가 필요 → **사용자 EnemyAI 코드에 1줄 조건**이 필요할 수 있음. 대안: 허수아비는 `EntityType::Enemy`인데 AI 감지 거리를 0으로 → 코드 수정 없음 (**이걸로 추천**: 튜토리얼 방 적의 감지 거리만 데이터로 0)
  → 그러려면 감지 거리를 엔티티별로 읽어야 함 = EnemyAI가 `kChaserDef` 대신 엔티티의 값을 봐야 함. 사용자 코드 확인 후 결정.

## 3. 의사코드 (AI)
```
// ---------- systems/RoomFlow_C.h ----------
enum class GamePhase { Title, Playing, GameOver, Victory }
enum class RoomState { Locked, Cleared }

class RoomFlow:                           // 흐름 판단만. 그리기 X (GDD 7장)
    GamePhase m_ePhase = Title
    int m_iRoomIndex = 0
    RoomState m_eRoomState = Locked

    Update(World&, Handle& hPlayer, Tilemap& tRoom, const MenuIntent&, dt):
        switch m_ePhase:
            Title / GameOver / Victory:
                if 확인 키(Enter): StartNewGame(...)          // 전체 초기화 -> Playing
            Playing:
                pPlayer = Get(hPlayer)
                if pPlayer.fHp <= 0: m_ePhase = GameOver; return
                if Locked and 적·보스가 하나도 없음: Cleared   // ForEachAlive로 셈 (Projectile·Drop 제외)
                    보스 방이었으면 Victory
                if Cleared and 플레이어가 rcDoor와 겹침: LoadRoom(m_iRoomIndex + 1)

    LoadRoom(i):  플레이어 외 전부 Destroy -> Flush -> tRoom = kRooms[i] 타일
                  -> 플레이어 위치 = 스폰, 속도 0 -> 적·보스 Create -> Locked (적 0이면 바로 Cleared: 튜토리얼 대비)
    StartNewGame: 전부 Destroy -> Flush -> 플레이어 새로 Create(풀 HP) -> LoadRoom(0) -> Playing

    읽기용: GetPhase(), GetRoomIndex(), GetRoomState()   (Render/HUD가 const로 읽음)

// ---------- core/MenuIntent_C.h ----------
MenuIntent { bConfirmPressed }            // Input이 Enter를 래치. RoomFlow만 읽음 (의도 분리 원칙 그대로)

// ---------- Game ----------
Run: Input.Poll(플레이어 의도, 메뉴 의도)
FixedUpdate:
    roomFlow.Update(...)                  // 맨 앞: 방 전환·승패
    if phase != Playing: 래치 해제하고 끝  // 타이틀·게임오버 중엔 게임 시뮬레이션 정지
    (기존 순서 그대로)

// ---------- render/Hud_C (UI) ----------
Playing:  왼쪽 위 HP 바 + 대시 게이지 바 / 오른쪽 위 방 이름 / 보스 방이면 화면 위 가운데 보스 HP 바
          튜토리얼 안내 문구 / Cleared면 "Door open ->" / 아이템 아이콘 줄 (아이템 작업 때)
Title:    "MAIN" + "Press Enter"
GameOver: 어둡게 + "GAME OVER" + "Press Enter to retry"
Victory:  "CLEAR!" + 걸린 시간 + "Press Enter"
기존 디버그 글자는 F1로 켜고 끄기 (기본 꺼짐)
```

## 4. 테스트 (AI, raylib 없이)
- 적 전멸 → Cleared / 적 남으면 Locked 유지
- Cleared + 문 겹침 → 다음 방, 플레이어 HP 유지, 이전 방 적 없음
- 플레이어 HP 0 → GameOver → 확인 → 튜토리얼, HP 풀
- 보스 방 보스 처치 → Victory
