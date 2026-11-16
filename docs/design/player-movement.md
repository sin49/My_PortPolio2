# 플레이어 이동 / 점프 / 타일 충돌 — 의사코드

> 상태: **합의됨 (2026-11-16)**. 이 문서 기준으로 구현함. 8장 질문은 답 기록.
> 관련: GDD 3장(게임 규칙), 7장(책임표·한 프레임 순서)

## 0. 확정된 결정 (2026-11-16)
| 항목 | 결정 |
|---|---|
| 타일 | 32px, 방 = **40 x 22칸** (1280 x 704px) |
| 화면 | 1280 x 720 창, 위·아래 **8px 검은 프레임**, 플레이 영역은 y = 8 ~ 712 |
| 점프 보조 | **코요테 타임 + 점프 버퍼 + 가변 점프** 전부 MVP에 포함 |

- 좌표계: 게임 로직은 **방 좌표**(플레이 영역 왼쪽 위 = (0,0), 단위 px, y는 아래가 +)만 씀.
  프레임 오프셋(8px)은 `Render`만 앎. 나중에 프레임 크기를 바꿔도 로직은 그대로.

## 1. 이전 코드(SBSBattery `Player.cs`)에서 가져올 것 / 버릴 것
| 기능 | 이전 구현 | 문제 | 이번 구현 |
|---|---|---|---|
| 점프 버퍼 | `jumpBufferTimer`를 `Update`(가변 dt)에서 감소, `FixedUpdate`에서 소비 | 타이머와 소비가 서로 다른 dt를 씀 → 프레임레이트에 따라 버퍼 길이가 달라짐 | 고정 틱 안에서 감소·소비 둘 다 처리 |
| 코요테 타임 | 전용 타이머 없음. `PlayerStat.jump` 플래그는 착지할 때만 true, 발판에서 걸어 내려가도 안 꺼짐 | 떨어지는 동안 **언제든** 점프 가능 (시간 제한 없는 코요테) | `coyoteTimer`: 땅에 있으면 리필, 공중이면 감소, 0보다 클 때만 점프 |
| 가변 점프 | `jumphold()`가 vy에 0.85를 곱함 + 코루틴이 `jumptime` 뒤에 vy를 0으로 | 코루틴 수명 관리(`StopCoroutine`), 착지 처리 코드 중복(`groundCheckEvnet`/`DelayBugCheck`) | 키를 뗄 때 상승 중이면 **한 번만** `vy *= kJumpCutMultiplier`. 코루틴 없음 |
| 상태 소유 | `PlayerStat.instance`(싱글톤)를 여러 곳에서 직접 수정 | 누가 값을 바꿨는지 추적 불가 | 점프 상태는 `MoveState` 하나에 모으고 이동 시스템만 수정 |

## 2. 데이터
```
// Input이 기록하는 의도 (위치/속도는 건드리지 않음)
struct MoveIntent {
    int  moveDir      = 0;      // -1, 0, +1
    bool jumpPressed  = false;  // 이번 틱에 눌림 (렌더 프레임에서 래치 → 틱이 소비)
    bool jumpHeld     = false;  // 누르고 있는 중 (가변 점프용)
}

// 물리 몸체 (플레이어·적 공용)
struct Body {
    Vec2 pos;            // AABB 왼쪽 위, 방 좌표 px
    Vec2 size;           // 예: 24 x 40
    Vec2 vel;            // px/s
    bool grounded = false;
}

// 점프 보조 상태 (플레이어 전용)
struct JumpState {
    float coyoteTimer = 0;
    float bufferTimer = 0;
    bool  isRising    = false;   // 점프로 상승 중 (가변 점프 컷 대상)
}

// 튜닝 값 → data/PlayerDef (코드에 매직넘버 X)
kGravity = 2400 px/s²,  kMaxFallSpeed = 900 px/s
kJumpSpeed = 760 px/s,  kJumpCutMultiplier = 0.5
kCoyoteTime = 0.10 s,   kJumpBufferTime = 0.12 s
// moveSpeed는 Stats 최종값에서 읽음 (기본값 + modifier)
```

## 3. 입력 래치 (렌더 프레임 ↔ 고정 틱)
```
// 렌더 프레임마다 (고정 틱이 0번 도는 프레임도 있음)
Input.Poll():
    intent.moveDir  = (D ? 1 : 0) - (A ? 1 : 0)
    intent.jumpHeld = IsKeyDown(Space)
    if IsKeyPressed(Space): intent.jumpPressed = true   // OR로 누적, 지우지 않음

// 고정 틱이 소비한 뒤
Game: 첫 틱이 끝나면 intent.jumpPressed = false
```
- 이유: 120Hz 모니터에선 고정 틱이 0번인 프레임이 생김 → 그 프레임에 누른 점프가 사라지면 안 됨.

## 4. 고정 틱: 점프 판단 (PlayerMotor)
```
PlayerMotor.Update(intent, body, jump, stats, dt):
    // 수평 이동 (가속 없이 즉시 — 손맛 조정은 나중에)
    body.vel.x = intent.moveDir * stats.moveSpeed

    // 타이머
    if body.grounded: jump.coyoteTimer = kCoyoteTime
    else:             jump.coyoteTimer -= dt
    if intent.jumpPressed: jump.bufferTimer = kJumpBufferTime
    else:                  jump.bufferTimer -= dt

    // 점프 실행: 버퍼 안에 입력이 있고, 코요테 시간 안이면
    if jump.bufferTimer > 0 and jump.coyoteTimer > 0:
        body.vel.y = -kJumpSpeed
        jump.bufferTimer = 0
        jump.coyoteTimer = 0      // ← 공중에서 다시 코요테 점프 못 하게 (이전 코드 문제)
        jump.isRising = true

    // 가변 점프: 상승 중에 키를 떼면 한 번만 감속
    if jump.isRising and not intent.jumpHeld and body.vel.y < 0:
        body.vel.y *= kJumpCutMultiplier
        jump.isRising = false
    if body.vel.y >= 0: jump.isRising = false   // 정점 지남
```

## 5. 고정 틱: 중력 + 타일 충돌 (Physics, 플레이어·적 공용)
```
Physics.Update(body, tilemap, dt):
    body.vel.y = min(body.vel.y + kGravity * dt, kMaxFallSpeed)

    // 축 분리: X 먼저, 그다음 Y (모서리에서 끼임 방지)
    body.pos.x += body.vel.x * dt
    ResolveAxis(body, tilemap, X)

    body.grounded = false
    body.pos.y += body.vel.y * dt
    ResolveAxis(body, tilemap, Y)

ResolveAxis(body, tilemap, axis):
    // AABB가 걸치는 타일 범위만 검사 (전체 맵 순회 X)
    for each tile (tx, ty) overlapping body box:
        if not tilemap.IsSolid(tx, ty): continue
        if axis == X:
            if body.vel.x > 0: body.pos.x = tx * 32 - body.size.x   // 오른쪽 벽
            if body.vel.x < 0: body.pos.x = (tx + 1) * 32           // 왼쪽 벽
            body.vel.x = 0
        else:
            if body.vel.y > 0: body.pos.y = ty * 32 - body.size.y; body.grounded = true  // 바닥
            if body.vel.y < 0: body.pos.y = (ty + 1) * 32                               // 천장
            body.vel.y = 0

Tilemap.IsSolid(tx, ty):
    범위 밖(0~39, 0~21 바깥) = 단단함 처리 (방 밖으로 못 나감, 문은 RoomFlow 단계에서)
```
- 터널링 체크: 틱당 최대 이동 = 900 / 60 = 15px < 타일 32px → 벽을 뚫지 않음. (`static_assert`로 고정)
- 오른쪽/아래 경계는 `floor((pos + size - ε) / 32)`로 계산 (딱 붙어 있을 때 옆 타일까지 겹치는 오판 방지).

## 6. 프레임 순서에서의 위치 (GDD 7장)
```
Input.Poll()                    // 렌더 프레임마다
for each fixed tick:
    PlayerMotor.Update(...)     // 의도 → 속도 (점프 보조 포함)
    Physics.Update(...)         // 중력, 이동, 타일 충돌, grounded
Render.Draw(const&)             // 프레임 8px 오프셋은 여기서만
```

## 7. 테스트 (doctest, raylib 비의존)
- 코요테: 발판 끝에서 걸어 나간 뒤 0.05초 후 점프 → 성공 / 0.2초 후 → 실패
- 버퍼: 착지 0.08초 전에 점프 입력 → 착지 틱에 점프 / 0.3초 전 → 무시
- 코요테 재사용 금지: 코요테 점프 후 공중에서 다시 입력 → 실패
- 가변 점프: 바로 떼면 최고 높이가 꾹 누를 때보다 낮음
- 충돌: 바닥 착지 시 `grounded`, 벽에 붙으면 `vel.x == 0`, 천장 박으면 `vel.y == 0`
- 경계: 타일에 딱 붙은 상태에서 옆 타일 오판 없음

## 8. 결정 기록
1. **`PlayerMotor` 신설 (GDD 7장 표에 반영함):** 의도 → 속도(점프 보조 포함)는 `PlayerMotor`,
   중력·이동·타일 충돌은 `Physics`(플레이어·적 공용). 사용자가 판단을 AI에 맡김.
   이유: Physics가 입력 의도와 점프 규칙까지 알면 적(Chaser)에 재사용할 때 플레이어 전용 분기가 생김.
2. **(B) 채택:** `Game`이 플레이어 `Body`/`JumpState`/`MoveIntent`를 임시로 소유. World(사용자 작성)가 생기면 옮김.
3. **플레이어 24 x 40px**로 시작, 해보고 문제 있으면 수정.
4. 방 데이터는 당분간 `data/TestRoom.h`의 문자열 배열(`#` 벽, `.` 빈칸). 파일(`assets/rooms/*.txt`) 로드는 RoomFlow 단계에서.
