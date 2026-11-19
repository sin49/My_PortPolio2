# Shooter + 투사체 + 보스 — 다음 단계 결정 (2026-11-19)

> 상태: **전부 구현 완료 (11/19)** — 투사체·Shooter·진영 정리·보스. 보스는 사용자가 "알아서" 결정 → AI가 `tBoss` 추가 후 구현

> 투사체·보스 기본 설계는 `boss-projectile.md`에서 **이미 합의됨** (World를 풀로 사용, 벽에 닿으면 삭제, 보스 패턴 조준 3연사 ↔ 8방향, HP 50% 아래 2페이즈).
> 이 문서는 **아직 안 정한 것**만 모음. 순서: 투사체 시스템 → Shooter → 보스 → (사운드 → README)

## 1. 결정할 것
| # | 질문 | 선택지 | AI 추천 |
|---|---|---|---|
| 1 | Shooter 움직임 | **A.** 거리 유지 (가까우면 뒤로, 멀면 다가옴, 적당하면 멈춰서 쏨) / **B.** 제자리 포탑 | **A.** Chaser와 다른 압박. 근접 캐릭터가 "파고들어야" 하는 이유가 생김 |
| 2 | Shooter 조준 | **A.** 플레이어 쪽으로 직선(대각선 포함) / **B.** 가로로만 | **A.** 보스 탄과 같은 코드. 점프로 피하는 맛 |
| 3 | Shooter 상태 저장 | **A.** 사용자 `EnemyAIState` 그대로 재사용 (Chase = "거리 맞추기", Attack = 1발 발사) / **B.** 새 구조체 | **A.** 상태 5개가 그대로 맞음. Entity 수정 없음 |
| 4 | Shooter AI 누가 | **A.** AI가 사용자 EnemyAI 모양 그대로 / **B.** 사용자 (Chaser와 거의 같은 FSM, 1시간 안쪽) | **A.** 사용자는 **Spatial Hash**에 시간 쓰는 게 포폴에 더 큼 (수치로 보여 주는 최적화). 같은 FSM을 두 번 짜는 건 새로 배우는 게 적음 |
| 5 | Shooter 크기·HP | 중간 크기 고정 (2대), 청록색 | 그대로. 크기 3종은 Chaser만 |
| 6 | 보스 상태 저장 | **A.** `BossAIState tBoss;` 새 필드 (`BossAI_C.h`에 정의, **Entity_U.h에 1줄 + include 1줄**) / **B.** `EnemyAIState`에 패턴 번호 추가 | **A.** 보스 상태(조준 예고·원형 예고 등)가 Chaser와 달라서 사용자 EnemyAI를 안 건드림 |
| 7 | 플레이어 공격으로 탄 지우기 | 지금 X (확장 예정: 패링) | 그대로 |
| 8 | 대시 무적으로 탄 피하기 | O (근접과 같은 `fInvulnTimer`) | 그대로. 대시가 회피기 역할을 확실히 함 |
| 9 | 적 진영 판정 정리 | `IsEnemySide(eType)` 한 함수 (Enemy·Shooter·Boss) → Combat·RoomFlow 클리어·HUD 남은 적·미니맵·EnemyAI가 같이 씀 | 그대로. 지금 `EntityType::Enemy` 직접 비교가 11군데 → 1군데로 |
| 10 | Spatial Hash 의사코드 시점 | **A.** **지금 같이** 써 둠 → AI가 투사체 짜는 동안 사용자는 Spatial Hash 구현 (병렬) / **B.** 보스 끝난 뒤 | **A.** 파일이 안 겹침 (`SpatialQuery_C` 내부만 교체, 호출부 그대로). 투사체가 생기면 엔티티 수가 늘어서 측정 효과도 잘 보임 |

## 2. 수치 (AI 제안, 플레이하며 조정)
```
kShooterDef { vSize {28, 38}, fMaxHp 20, fMoveSpeed 110,
              fDetectRange 560, fPreferMin 220, fPreferMax 380,   // 이 거리 사이면 멈춰서 쏨
              fWindupTime 0.5, fRecoverTime 1.4 }
kBulletDef  { vSize {12, 12}, fSpeed 320, fLifetime 3.0, fDamage 10 }   // Shooter·보스 공용 (투사체 1종 통일)
```
- 방 배치: 방 1에 Shooter 2, 방 2에 Shooter 3 (지금 Chaser로 대체 생성 중인 `SpawnKind::Shooter` 자리 + 발판 위)

## 3. 사용자 작업 (결정대로 가면)
- 결정 6-A: `Entity_U.h`에 `#include "systems/BossAI_C.h"` + `BossAIState tBoss;` (AI가 파일 만든 뒤 알려 줌)
- 결정 10-A: `spatial-hash.md` 의사코드 → `SpatialQuery` 내부 구현 (스펙 테스트 + 비교 횟수 측정 모드)
