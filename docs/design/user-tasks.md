# 사용자 작업 목록 (포폴 핵심) — 순서대로

> 2026-11-17 정리. 각 항목 = 의사코드 문서 + AI 스펙 테스트. **테스트 전부 통과 = 완료** → AI 리뷰·명명 정리(`_U`).
> 파일은 접미사 없이 만들어도 됨. 새 `.cpp`를 만들면 `cmake -S . -B build` 한 번 (자동 포함, 출력에 `~ 구현 포함: ON`).

| 순서 | 작업 | 문서 | 스펙 테스트 | 이걸 기다리는 AI 작업 |
|---|---|---|---|---|
| 1 ✅ | **Entity 필드 추가** (아래). `tAI`·enum 3개·`tProjectile` 완료, `tBoss`는 11/19 사용자 "알아서" 결정으로 AI가 2줄 추가 | 이 문서 | — | 보스 (완료) |
| 2 ✅ | **EnemyAI** (Chaser FSM, 11/19 완료) | `chaser.md` 3장 | `EnemyAITest_C.cpp` 9개 | Shooter·보스 AI (같은 모양으로) |
| 3 ✅ | **Stats** (기본값 + modifier, 11/19 완료) | `stats.md` 2장 | `StatsTest_C.cpp` 7개 | 아이템 |
| 4 ✅ | **Spatial Hash** (격자 자료구조, 11/19 완료 → `SpatialHash_U`) | `spatial-hash.md` 2·3장 | `SpatialHashTest_C.cpp` 7개 | SpatialQuery 연결·측정 (AI, 완료) |
| 5 → AI | ~~raylib 자원 RAII 래퍼~~ → 11/19 사용자 결정으로 **AI가 구현** (`audio/SoundAsset_C`, stress-sound.md 2장) | — | `static_assert` 4개 | — |

## 1. Entity_U.h 추가분 (EnemyAI 작업 때 같이)
```
#include "systems/EnemyAI_U.h"     // EnemyAIState (완료)
#include "systems/Projectile_C.h"  // ProjectileState, Faction (11/19 생성됨)

enum class EntityType { Player, Enemy, Drop, Boss, Projectile, Shooter }   // Boss, Projectile, Shooter 추가

EnemyAIState tAI;                  // 적 AI 상태 (bPassive 포함, chaser.md 3-2)
ProjectileState tProjectile;       // 투사체만 씀 (Projectile_C.h 생긴 뒤)

#include "systems/BossAI_C.h"      // BossAIState (11/19 생성됨)
BossAIState tBoss;                 // 보스만 씀 (shooter.md 결정 6-A)
```

## 2. 왜 이것들이 "노른자"인가 (포폴에 쓸 말)
- **World**: 세대 번호 핸들 + 고정 슬롯 → 삭제된 객체 참조를 구조적으로 차단 (C++ 수명 관리)
- **EnemyAI**: 명시적 FSM (enum + Enter + 타이머 전이), 의도만 쓰는 AI → 책임 분리
- **Stats**: 덮어쓰지 않는 스탯 (원본 const + 재계산) → 아이템 제거 시 되돌리기 코드 없음
- **Spatial Hash**: 넓은 맵 + 많은 엔티티에서 전체 순회 대비 비교 횟수·시간 측정 → 수치로 보여 주는 최적화
- **RAII 래퍼**: C API(raylib)의 수동 Load/Unload를 소멸자·이동 의미론으로 감싸 누수·이중 해제를 타입으로 막음

## 3. C++에서만 신경 쓰는 것: raylib 자원 수명 (RAII)
- raylib은 **C 라이브러리**라 `LoadTexture`/`UnloadTexture`, `LoadSound`/`UnloadSound`, `LoadMusicStream`/`UnloadMusicStream`을 **직접 짝 맞춰** 불러야 함. 안 부르면 누수, 두 번 부르면 이중 해제. 창(`CloseWindow`)·오디오 장치가 먼저 닫혀도 문제.
- C#은 GC + `IDisposable`/`using`. C++은 **소멸자에서 자동 해제(RAII)** + 복사 금지/이동 허용(Rule of 5)으로 해결.
- 할 것 (작음, 1시간 안쪽): `class SoundAsset { Sound m_tSound; ... }`
  - 생성자에서 Load, 소멸자에서 Unload
  - 복사 생성/대입 `= delete` (두 객체가 같은 자원을 해제하는 것 방지)
  - 이동 생성/대입: 자원을 넘겨받고 원래 쪽은 "빈 상태"로 (빈 상태는 소멸자에서 Unload 안 함)
  - `Game`이 창보다 먼저 파괴되는 구조(`main_C.cpp`의 `{ }` 블록)가 이미 있어서 순서 문제도 해결돼 있음
- 사운드 단계에서 쓰므로 그때 의사코드 + `static_assert(!std::is_copy_constructible_v<SoundAsset>)` 같은 검사 제공.
