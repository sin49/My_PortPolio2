# HP · 피격 판정 · SpatialQuery — 설계

> 상태: **합의됨 (2026-11-18)** — 1~5 전부 A. 단 5는 사용자 결정으로 **넉백 + 깜빡임 + 파티클**(경직·무적은 나중, 아래 6장).
> 1(Events)은 듣는 시스템이 생기는 수·목에 추가. 2(16칸)는 부족하면 늘림 → 많이 몰리면 스트레스 테스트·최적화 거리 = 포폴 재료.
> 합의 전: 제안 (2026-11-18). 기준: GDD 6장(근접 판정), 7장(Combat 책임), CLAUDE.md(SpatialQuery 필수)
> 범위: 플레이어 공격이 **허수아비 적**(AI 없음)을 맞혀 HP 감소 → 넉백 + 깜빡임 → 0이면 삭제. Chaser AI는 다음 단계.

## 1. 결정할 것
| # | 질문 | 선택지 | AI 추천 |
|---|---|---|---|
| 1 | `Events`(DamageEvent)를 지금 만들까 | **A.** 나중에 (목요일 아이템 `OnKill` 때) — Combat이 HP를 직접 깎음 / **B.** 지금 | **A.** 지금은 듣는 쪽이 없음. HP 차감·사망 표시는 원래 Combat 책임(GDD 7장). 들을 시스템이 생기면 그때 Events 추가 → GDD 7장 "DamageEvent 발행"은 목요일로 표기 |
| 2 | 맞은 적 목록 `alreadyHit` 컨테이너 | **A.** 고정 배열 16칸 + 개수 / **B.** `std::vector<Handle>` | **A.** 할당 없음 + Entity 복사가 가벼움. 한 번 휘둘러 16마리 넘게 맞을 일 없음 (넘으면 그 뒤는 무시) |
| 3 | 플레이어 HP 0 | **A.** 삭제 안 함, HP 0으로 둠 → 수요일 RoomFlow가 게임 오버 처리 / **B.** 바로 삭제 | **A.** 플레이어 핸들이 무효가 되면 Render·Input이 할 일이 없어짐. 승패는 RoomFlow 책임 |
| 4 | 누가 누구를 때리나 | **A.** `eType`으로: Player ↔ Enemy만 / **B.** 지금 `iTeam` 필드 | **A.** 지금은 충분. PvP 때 `iTeam`으로 바꾸면 `IsHostile()` 함수 하나만 수정 |
| 5 | 피격 손맛 | **A.** 넉백 + 깜빡임만 (GDD 6장 MVP) / **B.** + 경직·무적 시간 | **A.** 넉백은 Motor의 감속(0.19초)으로 자연스럽게 미끄러짐. 무적은 Chaser가 플레이어를 때릴 때 필요하면 추가 |

## 2. 사용자 작업 (결정 1~5 추천안 기준, 2개)
### 2-1. `World_U.h`: const 버전 `ForEachAlive`
목적: `const World&`로도 순회 (Render가 적을 그리고, SpatialQuery가 겹침을 찾을 때). World를 못 바꾸는 순회.
```
template <typename Fn>
ForEachAlive(fnVisit) const:                 // [C++] 함수 이름 뒤 const. 기존 것 바로 아래에 하나 더
    for uIndex in 0..kCapacity-1:
        tSlot = m_vecSlots[uIndex]           // [C++] const Slot& tSlot (const 함수 안이라 const 참조만 가능)
        if tSlot.bAlive: fnVisit(Handle{uIndex, tSlot.uGen}, tSlot.tValue)
                                             // 콜백은 const Entity&를 받게 됨
```
- 이름이 같고 const만 다른 함수 2개 = C#엔 없는 "const 오버로드". 컴파일러가 World가 const인지 보고 골라 줌 (`Get`과 같은 방식).

### 2-2. `Entity_U.h`: 필드 1개
```
HitState tHit;      // 깜빡임 타이머 (Combat_C.h에 AI가 정의함. include는 이미 있음)
```

## 3. 데이터 (AI)
```
// data/PlayerDef_C.h 에 추가
fMaxHp = 100, fAttackDamage = 10, fKnockbackSpeed = 320, fKnockbackUp = 220

// data/EnemyDef_C.h (새 파일) — Chaser 수치. 오늘은 허수아비로만 사용
kChaserDef { vSize = {28, 36}, fMaxHp = 30, fMoveSpeed = 140 (다음 단계) }

// data/TestRoom_C.h 에 추가
kTestRoomEnemySpawns = 바닥 위 3곳
```

## 4. 의사코드 (AI)
```
// ---------- systems/SpatialQuery_C.h ----------
// 호출부(Combat, 나중의 AI)는 이 인터페이스만 앎. 금요일에 내부를 Spatial Hash로 바꿔도 호출부 그대로.
class SpatialQuery:
    SpatialQuery(const World& tWorld)       // 참조 보관 (Game이 둘 다 소유, 수명 같음)
    Rebuild()                               // MVP: 아무것도 안 함. 해시 때 격자 재구축. Game이 Physics 직후 호출
    QueryNearby(rcArea, vecOut&) const:     // 결과는 호출자 버퍼에 (매 틱 재사용 -> 할당 없음)
        vecOut.clear()
        world.ForEachAlive(const): if Overlaps(rcArea, 몸 사각형): vecOut.push_back(h)

// ---------- systems/Combat_C.h ----------
struct HitState { float fFlashTimer = 0 }
AttackState에 추가: std::array<Handle, 16> arrAlreadyHit; int iAlreadyHitCount = 0
    공격 시작(Ready -> Active) 때 iAlreadyHitCount = 0

Combat.ResolveHits(World&, const SpatialQuery&, dt):     // 모든 공격 단계 갱신이 끝난 뒤 호출
    world.ForEachAlive(hAttacker, tAttacker):
        tAttacker.tHit.fFlashTimer = max(0, fFlashTimer - dt)
        if tAttacker.tAttack.ePhase != Active: continue
        spatial.QueryNearby(tAttacker.tAttack.rcHitbox, m_vecHits)   // m_vecHits: Combat 멤버 버퍼 (reserve)
        for hTarget in m_vecHits:
            pTarget = world.Get(hTarget)
            if !pTarget or !IsHostile(tAttacker, *pTarget): continue
            if pTarget->fHp <= 0: continue                 // 이번 틱에 이미 죽음 (삭제 예약 중, world.md 4장 주의점)
            if AlreadyHit(tAttacker.tAttack, hTarget): continue
            기록(hTarget)
            pTarget->fHp -= kPlayerDef.fAttackDamage        // 목요일: Stats의 최종 공격력으로
            pTarget->tBody.vVel = {iFacing * 넉백속도, -넉백위}
            pTarget->tHit.fFlashTimer = 0.1
            if pTarget->fHp <= 0 and pTarget->eType != Player: world.Destroy(hTarget)   // 예약만 (결정 3)

IsHostile(a, b): (a Player && b Enemy) || (a Enemy && b Player)      // PvP 때 iTeam 비교로 교체

// ---------- Game ----------
Game(): 플레이어 생성(fHp = kPlayerDef.fMaxHp) + kTestRoomEnemySpawns마다 Enemy 생성(kChaserDef)
FixedUpdate:
    Motor(전부) -> Physics(전부) -> spatial.Rebuild() -> 공격 단계(전부) -> combat.ResolveHits -> Flush
    // Motor 속도: Player면 kPlayerDef, Enemy면 kChaserDef (오늘 적은 의도 0이라 감속만 함 = 넉백 후 미끄러짐)

// ---------- Render ----------
const ForEachAlive로 Enemy 그리기 (빨간 사각형, fFlashTimer > 0이면 흰색)
플레이어도 fFlashTimer 반영. 디버그 줄에 플레이어 HP 추가
```
- ForEachAlive 안에서 `Get(hTarget)`으로 다른 엔티티를 고치는 건 안전: 칸 추가·삭제가 아니라 값 수정이라서. 삭제는 `Destroy` 예약만.
- 오늘 적의 Motor 의도가 비어 있어서 `iFacing`이 안 바뀜 → 문제 없음 (Chaser 때 AI가 의도를 씀).

## 4-1. 피격 파티클 (사용자 추가 요청: 맞은 반대쪽으로 하얀 파편)
- 파티클은 **게임 상태가 아님**(판정에 영향 없음) → World에 넣지 않음. `Particles`가 자기 고정 풀(256개)을 소유.
- Combat은 파티클을 모름. 판정 결과만 `HitReport{vPos, iDir}`로 호출자 버퍼에 적어 줌 → Game이 Particles에 전달.
  (= Events 없이 "결과 목록"으로 넘기는 방식. 수·목에 Events가 생기면 OnDamage 구독으로 바꿀 수 있음)
```
// ---------- render/Particles_C.h (raylib 없음, 그리기는 Render) ----------
Particle { vPos, vVel, fLife, fMaxLife }
class Particles:
    std::array<Particle, 256>, 살아 있는 개수 대신 fLife > 0 으로 구분 (빈 칸 찾기: 순서대로 훑음, 다 차면 무시)
    std::mt19937 m_tRng (고정 시드 -> 재현 가능, 전역 아님)
    SpawnHitSparks(vPos, iDir): 6~8개, 방향 = iDir 쪽 ±35도, 속도 180~360, 수명 0.2~0.35초
    Update(dt): 위치 += 속도·dt, 중력 약하게, fLife -= dt
Render: fLife/fMaxLife 비율로 크기(3px -> 1px)·투명도 줄이며 흰 사각형
Game.FixedUpdate: combat.ResolveHits(..., m_vecHitReports) -> 각 리포트마다 particles.SpawnHitSparks -> particles.Update(dt)
```
- `iDir` = 공격자 → 맞은 쪽 방향(공격자 facing). "맞은 반대쪽으로 튐" = 맞은 적 뒤쪽으로 날아감.
- 위치 `vPos` = 히트박스와 몸이 겹친 사각형의 가운데.

## 5. 테스트 (AI, raylib 없이)
- SpatialQuery: 겹치는 것만 반환, 죽은(Flush된) 엔티티 제외
- Combat: 플레이어 공격 Active + 적 겹침 → HP 1회 감소 / 같은 휘두름에 두 번 안 맞음 / 다음 휘두름엔 다시 맞음
- HP 0 → Destroy 예약 → Flush 후 Get == nullptr / 플레이어는 HP 0이어도 삭제 안 됨 / 적끼리는 안 때림
- 맞으면 HitReport 1개 (방향 = 공격자 facing)

### 4-2. 파티클 조정 (실행 후 사용자 피드백)
- "티가 너무 안 남, 죽을 때만 보이는 것 같음" → 원인 추정: 3px로 작고, 맞는 순간 적이 **흰색으로 깜빡여서 흰 파편이 묻힘**
- 수정: 때릴 때 10~14개 · 6px · 수명 0.25~0.45초 / **죽을 때(`HitReport.bKilled`) 28~36개 · 8px · 넓게 한 번 더**
- 모양 값은 `SparkShape` 묶음 2개(`kHitSparks`, `kDeathBurst`)로 `Particles_C.cpp` 위쪽에서 조정

## 6. 나중에 (지금 안 함, 기록만)
- **파편 물리 (사용자 요청):** 바닥·벽에 튕기거나 붙기. 작업량은 크지 않음 (Particles가 `const Tilemap&`을 받아 점 하나씩 타일 검사,
  닿으면 속도 반전 × 감쇠 또는 0 + 붙은 채 페이드). 연출 전용이라 Physics(엔티티 AABB)와 별도로 둠
- **경직·무적:** 작업량은 작음(각각 `HitState`에 타이머 1개 + 검사 몇 줄). 미룬 이유는 복잡해서가 아니라 **조율 문제**
  (경직이 길면 적에게 갇혀 계속 맞는 "스턴락", 무적이 길면 긴장감 없음) → Chaser가 플레이어를 때려 봐야 수치를 정할 수 있음.
  `HitState`를 구조체로 둔 이유: 나중에 `fStunTimer`, `fInvulnTimer`를 **Entity 수정 없이** Combat_C.h 안에서 추가.
- **조합 키 (예: Shift 빠르게 2번 = 돌진 공격):** 지금 구조에서 확장 방법
  1. `Input`이 키를 누른 **틱 번호**를 짧은 기록(최근 N개)으로 남김
  2. `CommandRecognizer`(새 시스템)가 기록에서 패턴(더블탭, ↓→+X 등)을 찾아 의도에 씀 → `AttackIntent.eKind = DashAttack`
  3. Combat은 `eKind`별 공격 데이터(히트박스·시간·이동)를 테이블에서 꺼냄
  - 핵심: 패턴 인식은 **의도 앞단**에서 끝남 → Motor·Combat은 "무슨 공격인지"만 받음. 적 AI도 같은 `eKind`를 쓸 수 있고, PvP 네트워크에서도 의도만 보내면 됨.
  - 시간 판정은 고정 틱 번호로 (프레임 시간 X) → 재현 가능.
