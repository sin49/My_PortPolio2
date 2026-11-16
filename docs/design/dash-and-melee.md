# 대시(달리기) / 근접 공격 — 의사코드

> 상태: **합의됨 (2026-11-17)**. 9장 답 기준으로 구현함.
> 관련: `player-movement.md`, GDD 6장(근접 판정), 7장(책임표)

## 0. 정의 (2026-11-17 사용자 결정)
- **대시 = 누르고 있는 동안 빨리 달리기.** 무적·순간이동 없음.
- 대시 게이지가 있고, 누르는 동안 닳음. 0이 될 때까지 계속 가능.
- 다 쓰면(**탈진**) 게이지가 **절반 이상 회복될 때까지** 다시 못 씀.
- 근접 공격 = 플레이어 앞에 박스(히트박스)를 잠깐 띄움. 적이 생기기 전까지는 판정 없이 보이기만.
- 나중에(지금 X): 대시 후 점프, 대시 공격 같은 응용 조작.

## 1. 입력 (의도) 확장 — 이동/공격 의도 분리 (9장 5번)
```
struct MoveIntent {            // PlayerMotor만 읽음
    int  moveDir       = 0
    bool jumpPressed   = false   // 래치
    bool jumpHeld      = false
    bool dashHeld      = false   // 누르고 있는 중
}
struct AttackIntent {          // Combat만 읽음
    bool attackPressed = false   // 래치
}
// Game: 첫 고정 틱 후 jumpPressed, attackPressed를 false로
```

## 2. 대시 (PlayerMotor 안)
```
struct DashState {
    float gauge     = 1.0     // 0 ~ 1 (비율). 최대치는 나중에 Stats로 뺄 수 있음
    bool  exhausted = false   // 탈진: gauge >= 0.5 될 때까지 대시 금지
    bool  active    = false   // 이번 틱에 대시 중인지 (Render·디버그용)
}

// PlayerDef 추가 값
dashSpeedMultiplier = 1.8      // 260 -> 468 px/s
dashDrainPerSec     = 0.5      // 꽉 찬 게이지로 2초 달림
dashRegenPerSec     = 0.4      // 0 -> 0.5 회복까지 1.25초
dashRecoverRatio    = 0.5      // 탈진 해제 기준

UpdateDash(intent, dash, dt):
    wantsDash = intent.dashHeld and intent.moveDir != 0 and not dash.exhausted
    if wantsDash:
        dash.active = true
        dash.gauge -= dashDrainPerSec * dt
        if dash.gauge <= 0:
            dash.gauge = 0; dash.exhausted = true; dash.active = false
    else:
        dash.active = false
        dash.gauge = min(1, dash.gauge + dashRegenPerSec * dt)
        if dash.exhausted and dash.gauge >= dashRecoverRatio:
            dash.exhausted = false

// 기존 수평 관성에서 목표 속도만 바뀜
targetSpeed = moveDir * moveSpeed * (dash.active ? dashSpeedMultiplier : 1)
```
- 대시를 떼면 목표 속도가 내려가고, 기존 감속(관성)으로 자연스럽게 느려짐.
- 이동 입력 없이 대시 키만 누르면 게이지 안 닳음.

## 3. 바라보는 방향 (facing)
```
// PlayerMotor가 관리. 공격 방향에 씀.
if intent.moveDir != 0: facing = intent.moveDir     // 멈춰도 마지막 방향 유지
```
- 플레이어 전용 상태를 한 묶음으로: `PlayerMotorState { JumpState jump; DashState dash; int facing = 1; }`

## 4. 근접 공격 (Combat)
```
enum class AttackPhase { Ready, Active, Recover }

struct AttackState {
    AttackPhase phase = Ready
    float timer = 0
    Rect  hitbox                        // Active일 때만 의미 있음 (Render가 그림)
    // TODO(적 추가 후): std::vector<Handle> alreadyHit  -> 한 번 휘두를 때 같은 적 1회만
}

// PlayerDef 추가 값
attackActiveTime  = 0.15     // 히트박스 유지
attackRecoverTime = 0.20     // 다음 공격까지 대기
attackSize        = (40, 32) // 범위 modifier는 Stats 단계에서

Combat.UpdatePlayerAttack(intent, body, facing, attack, dt):
    switch attack.phase:
        Ready:
            if intent.attackPressed:
                attack.phase = Active; attack.timer = attackActiveTime
                // alreadyHit.clear()
        Active:
            attack.hitbox = 몸 앞쪽 사각형 (facing 방향, 세로 가운데 맞춤)   // 매 틱 다시 계산 -> 몸을 따라감
            // TODO(적 추가 후): for h in SpatialQuery.QueryNearby(hitbox):
            //                       if h not in alreadyHit and 겹침: DamageEvent 발행; alreadyHit.add(h)
            attack.timer -= dt
            if attack.timer <= 0: attack.phase = Recover; attack.timer = attackRecoverTime
        Recover:
            attack.timer -= dt
            if attack.timer <= 0: attack.phase = Ready

hitbox 계산:
    x = facing > 0 ? body.pos.x + body.size.x : body.pos.x - attackSize.w
    y = body.pos.y + (body.size.y - attackSize.h) / 2
```

## 5. 메모리 관리 관점 (근접은 왜 간단한가)
- 근접 히트박스는 **엔티티가 아님**. 플레이어의 `AttackState` 안의 값(사각형 + 타이머)이라 생성/삭제가 없음.
  → 원거리 투사체처럼 풀·수명·삭제 예약이 필요 없음.
- 단 하나 주의: `alreadyHit`에는 **적 포인터가 아니라 Handle**을 저장. 휘두르는 도중 적이 죽어서
  슬롯이 재사용돼도, 핸들은 세대 번호가 달라서 엉뚱한 적과 섞이지 않음. (World 완성 후 적용)

## 6. 고정 틱 순서
```
PlayerMotor.Update   // 관성 + 대시 + 점프 + facing
Physics.Update       // 중력 / 이동 / 충돌
Combat.Update        // 공격 상태·히트박스 (이동이 끝난 위치 기준)
```

## 7. Render
- Active 동안 히트박스를 반투명 사각형으로 표시.
- 디버그 텍스트에 대시 게이지(탈진이면 표시), 공격 단계 추가. 게이지 HUD 바는 나중에 HUD 단계에서.

## 8. 테스트
- 대시: 누르면 최고 속도가 1.8배 / 2초 넘게 누르면 탈진 / 탈진 중 누르면 일반 속도 / 0.5 회복 후 다시 가능
- 대시: 이동 입력 없이 대시 키만 → 게이지 유지
- 공격: 누르면 Active → 0.15초 후 Recover → 0.2초 후 Ready / Active·Recover 중 입력은 무시
- 공격: facing 왼쪽이면 히트박스가 몸 왼쪽

## 9. 결정 기록
1. **키:** `Z` 점프 / `X` 공격 / `Shift` 대시(누르고 있기) / `←→` 이동
2. **공중 대시:** 적용
3. **공격 중 이동:** 안 멈춤. 이유(사용자): 나중에 대시 공격 만들기 수월함
4. **공격 버퍼:** 넣음 (점프 버퍼와 같은 방식, 0.12초). Recover가 끝나는 틱에 버퍼가 남아 있으면 바로 Active
5. **의도 분리 (1장 수정):** 사용자가 "한 구조체에 이동+공격을 넣으면 책임 분리가 망가짐"을 우려 →
   `Intent`로 합치지 않고 `MoveIntent`(이동·점프·대시, PlayerMotor만 읽음) + `AttackIntent`(공격, Combat만 읽음)로 분리.
   Input·EnemyAI가 두 개를 각각 기록함.
