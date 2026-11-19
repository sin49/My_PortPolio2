# 보스 + 투사체 풀 — 설계

> 상태: **구현 완료 (11/19)** (투사체·Shooter·보스). 합의 (2026-11-17) — 결정 1~5 전부 AI 추천. 사용자 결정: 보스 AI는 AI가 짜되 **사용자 EnemyAI의 FSM 방식 그대로**, 보스 공격 = **투사체**, 투사체는 **풀링**.
> 구현 순서: 사용자 EnemyAI 완성 → (그 코드 스타일을 보고) AI가 보스 FSM 작성. 투사체 시스템은 Entity 필드만 있으면 먼저 가능.

## 1. 결정할 것
| # | 질문 | 선택지 | AI 추천 |
|---|---|---|---|
| 1 | 투사체 풀을 어디에 | **A.** **World를 그대로 풀로 사용** (`EntityType::Projectile`) / **B.** 별도 `ProjectilePool` (고정 배열 + 빈 칸 목록) | **A.** World가 이미 "고정 슬롯 + 빈 칸 재사용 + 세대 핸들 + 삭제 예약" 풀임. 투사체는 생성·삭제가 가장 잦은 엔티티라 **사용자가 만든 풀이 할당 0으로 버티는 걸 보여 주는 실사용 사례**가 됨. 맞음 판정도 SpatialQuery를 그대로 탐. B는 같은 걸 두 번 만드는 것 |
| 2 | 보스 종류 표시 | **A.** `EntityType::Boss` 추가 / **B.** Enemy + 종류 필드 | **A.** 방 클리어·승리 판정이 "Boss가 죽었나"로 단순해짐. 적대 판정은 `IsEnemySide(Enemy, Boss)`로 묶음 |
| 3 | 투사체가 벽에 닿으면 | 즉시 삭제 (+ 작은 파편) | 그대로 |
| 4 | 보스 패턴 | **조준 3연사**(플레이어 쪽 부채꼴 3발) ↔ **8방향 원형탄** 번갈아. HP 50% 아래면 Recover 짧아짐 (2페이즈) | 그대로. 간단하지만 "패턴 + 페이즈"가 보임 |
| 5 | 플레이어도 투사체를 쏘나 | 지금 안 함. 단 투사체에 "누가 쐈나(진영)"를 넣어 두면 나중에 원거리 무기·PvP 때 그대로 씀 | 그대로 |

## 2. 사용자 작업 (결정 1-A, 2-A 기준) — EnemyAI 작업 때 같이
```
// src/core/Entity_U.h
#include "systems/Projectile_C.h"          // ProjectileState (AI가 정의)

enum class EntityType { Player, Enemy, Drop, Boss, Projectile }   // Boss, Projectile 추가
ProjectileState tProjectile;               // Entity 안. 투사체만 씀
```

## 3. 데이터 (AI)
```
// data/BossDef_C.h
kBossDef { vSize {64, 72}, fMaxHp 300, fMoveSpeed 60,
           fAimedWindup 0.5, fRingWindup 0.7, fRecover 1.2, fRecoverPhase2 0.7,
           iAimedCount 3, fAimedSpreadDeg 12, iRingCount 8 }
// data/ProjectileDef_C.h
kBossBulletDef { vSize {12, 12}, fSpeed 320, fLifetime 3.0, fDamage 10 }
```

## 4. 의사코드 (AI)
```
// ---------- systems/Projectile_C.h ----------
struct ProjectileState { float fLife = 0; float fDamage = 0; Faction eOwner = Faction::Enemy }   // 진영. EntityType을 쓰면 Entity_U.h와 include 순환 -> 별도 enum Faction (11/19)

class Projectiles:
    Spawn(World&, vPos, vDir, const ProjectileDef&, eOwner) -> Handle
        = world.Create(Entity{ eType = Projectile, tBody{중심 vPos, vSize, vVel = vDir * fSpeed}, tProjectile{...} })
        // World가 꽉 차면 throw (world.md 결정 4). 보스 탄은 동시에 최대 ~30발이라 256칸에 여유
    Update(World&, const Tilemap&, dt):                 // 투사체 전용 이동 (중력 X, Motor X)
        ForEachAlive(h, e): if e.eType != Projectile: skip
            e.tBody.vPos += e.tBody.vVel * dt
            e.tProjectile.fLife -= dt
            if fLife <= 0 or 중심이 Solid 타일: world.Destroy(h)  (+ 벽이면 HitReport처럼 WallReport -> 작은 파편)

// ---------- Combat ----------
ResolveProjectileHits(World&, const SpatialQuery&, vecOutReports):
    ForEachAlive(hBullet, tBullet): if Projectile 아님 or 이미 삭제 예약(fLife <= 0): skip
        spatial.QueryNearby(몸 사각형) -> 대상마다:
            적대(진영 eOwner vs 대상)이고 HP > 0 이면: HP -= fDamage, 넉백(탄 진행 방향), 깜빡임, HitReport
            world.Destroy(hBullet); 1명 맞으면 끝 (관통 X)
IsHostile 정리: 진영(Player 쪽 / Enemy 쪽)으로 비교. Boss = Enemy 쪽, Projectile = eOwner 쪽

// ---------- systems/BossAI_C (사용자 EnemyAI와 같은 모양) ----------
enum class BossState { Idle, AimedWindup, AimedFire, RingWindup, RingFire, Recover }
Update(World&, Handle hPlayer, Projectiles&, dt):
    Idle:         플레이어 감지 -> Enter(AimedWindup)
    AimedWindup:  멈춤, timer 끝 -> Enter(AimedFire)
    AimedFire:    플레이어 방향 기준 -12°, 0°, +12° 로 3발 Spawn -> 다음 패턴 = Ring -> Enter(Recover)
    RingWindup:   멈춤, timer 끝 -> Enter(RingFire)
    RingFire:     45°마다 8발 Spawn -> 다음 패턴 = Aimed -> Enter(Recover)
    Recover:      플레이어 쪽으로 천천히 걸음 (iMoveDir), timer 끝 -> 다음 패턴의 Windup
    Enter(Recover): HP <= 50% 이면 fRecoverPhase2
    // 상태 저장은 EnemyAIState 재사용 가능 여부를 사용자 코드 보고 결정 (enum이 달라서 BossAIState 따로일 가능성 높음)

// ---------- Game.FixedUpdate 순서 ----------
EnemyAI -> BossAI -> Motor(Player·Enemy·Boss만) -> Physics(투사체 제외) -> Projectiles.Update
-> Spatial.Rebuild -> 공격 단계 -> ResolveHits(근접) -> ResolveProjectileHits -> 파티클 -> Flush
```
- Motor·Physics가 투사체를 건너뛰게 Game 루프에 종류 검사 추가 (투사체는 직선 이동만).
- Render: 투사체 = 주황 원(작은 사각형), 보스 = 큰 보라 사각형 + 화면 위 보스 HP 바, Windup 중 깜빡임.

## 5. 테스트 (AI)
- 투사체: 직진 / 수명 끝나면 삭제 / 벽 타일에 닿으면 삭제 / 적 진영 탄은 플레이어만 맞힘(보스·적 무시) / 맞으면 탄 삭제(관통 X)
- 풀: 탄 1000발을 생성·삭제 반복해도 `GetAliveCount`가 정상, 용량 안 넘음 (= World 재사용 확인)
- 보스: 패턴 순서 Aimed → Ring → Aimed / 3발·8발 개수 / HP 50% 아래 Recover 짧아짐

## 6. 구현 메모 (11/19)
- 상태 저장 = `BossAIState tBoss` (결정 6-A). 발사는 `AttackIntent`(`bFirePressed`·`vAim`·`iShotCount`·`fSpreadDeg`)로 의도만 → `Projectiles.FireFromIntents`가 순회 밖에서 생성 (Shooter와 같은 길)
- 수치 `kBossDef` (`EnemyDef_C.h`): 64x72, HP 100 (사용자 요청으로 300에서 낮춤), 이동 60, 감지 900, 조준 예고 0.5 / 3발 24°, 원형 예고 0.7 / 8발, 쉼 1.2 (2페이즈 0.7)
- 화면: 보라 큰 몸 + 눈(바라보는 쪽), 예고 중 깜빡임, 원형 예고는 몸 둘레 고리, 2페이즈는 붉은 보라. 화면 위 보스 HP 바 + 50% 눈금
- 테스트 `BossAITest_C.cpp` 5개: 패턴 순서·탄 수, 예고 시간·조준 방향, 2페이즈 쉼, 의도만 씀, 감지 거리·팩토리
