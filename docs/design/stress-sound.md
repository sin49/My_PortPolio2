# 맵 축소 + 스트레스 소환 + 효과음 (2026-11-19)

> 상태: **구현 완료 (11/19)**. 사용자가 조건을 직접 정해 줌 → 의사코드 합의 단계 없이 바로 구현.
> 사운드는 원래 사용자 선택 작업(RAII 래퍼, user-tasks.md 5)이었으나 사용자가 "AI가 처리"로 결정.

## 0. 맵 축소
- 방 1: 100 x 22 → **72 x 22**, 방 2: 120 x 30 → **88 x 26** (가로 약 0.72배, 방 2는 높이도 4칸 줄임)
- 발판·적 위치는 같은 비율로 옮김. 적 수·종류는 그대로 (방 1: 7, 방 2: 10). 발판 높이 차이는 유지 (점프 난이도 그대로)

## 1. 스트레스 소환 (F2) — 사용자 조건
| 조건 (사용자) | 구현 |
|---|---|
| 클리어 안 된 방에서만 | Game이 `RoomFlow.GetRoomState() == Locked`일 때만 호출 |
| 위치 랜덤, 플레이어 완전 근처만 제외 | 방 테두리 안 랜덤 사각형, 벽과 겹치면 다시 뽑음, 플레이어 중심 220px 안 제외 (최대 40번 시도). 공중이면 떨어짐 |
| 근접 작은 적만 | `SpawnKind::Chaser` + `EnemySize::Small` (사용자 EnemyAI가 그대로 움직임) |
| 제한 없음 | 개수 제한 없음 (11/19 2차: 남겨 두던 48칸도 제거). **World 풀(9999칸)이 꽉 찰 때까지**. 꽉 차도 안 죽음: 투사체·드랍은 빈 칸이 없으면 건너뜀 (`Create`의 throw는 치명적 오류용이라 피함). HUD에 `WORLD POOL FULL 9999 / 9999` |
| 미니맵 밑 적 표시, 10 넘으면 빨간 박스 × 숫자 | 남은 적 ≤ 10: 기존 칸 표시 (소환으로 늘면 칸도 늘어남) / > 10: `■ x 23` |

```
Input.PollDebug:  F2 눌림(또는 꾹 = OS 키 반복) -> DebugIntent.iSpawnEnemyPresses++
Game.FixedUpdate (RoomFlow 다음, AI 전 = 순회 밖):
    if 방 == Locked: 누른 횟수만큼 StressSpawner.SpawnSmallChaser(World, 방, 플레이어)
StressSpawner.SpawnSmallChaser:
    if World 풀이 꽉 참: return false   (11/19 2차: 남겨 두던 48칸 제거)
    40번까지: 랜덤 사각형 -> 벽과 겹침? / 플레이어 220px 안? -> 다시
              통과 -> World.Create(MakeEnemyEntity(Chaser, Small)) -> true
```
- 파일: `systems/StressSpawner_C` (raylib 모름 → 테스트 2개), `core/DebugIntent_C.h`
- Spatial Hash 측정(F1 패널 비교 횟수) 때 이걸로 엔티티 수를 늘려서 전체 순회 vs 해시 수치 비교

## 2. 효과음 (파일 없이 합성)
- **다운로드 없음**: 효과음 8개를 코드로 합성 (사각파·삼각파·잡음 + 주파수 미끄러짐 + 음 계단 + 감쇠). 라이선스·파일 관리 불필요
  - Swing·Hit·Kill·Hurt·Shoot·Pickup·Dash·RoomClear
- 구조 (책임 분리):
  | 파일 | 하는 일 | raylib |
  |---|---|---|
  | `audio/SfxSynth_C` | `SfxId` → 16비트 샘플 (레시피 테이블) | X (GameLogic, 테스트 2개) |
  | `audio/SoundAsset_C` | **RAII**: 생성자 Load / 소멸자 Unload, 복사 delete, 이동 시 원래 쪽 빈 상태. `static_assert`로 복사 금지·noexcept 이동 검사. `AudioDeviceScope`도 같은 방식 | O |
  | `audio/Audio_C` | 소리 8개 소유, `Queue(id)` → 프레임 끝 `PlayQueued` (같은 소리는 프레임당 1번, 피치 ±6%) | O |
- 수명 순서 (`main_C.cpp`): `AudioDeviceScope` → `Game` 순서로 선언 → 파괴는 반대(Game의 Sound 먼저 해제 → 장치 닫기). C#의 `using` 중첩과 같음
- 트리거 (Game.QueueTickSounds, 연출이라 판정과 무관): HitReport(플레이어 피격 = Hurt / 처치 = Kill / 나머지 = Hit), 발사 횟수 > 0, 줍기, 플레이어 대시·휘두름 **시작 순간**(지난 틱 상태와 비교), 방 클리어

## 3. 2차 (11/19, 사용자 요청: "극한으로, 망가지는 정도 확인용")
- **디버그 모드(F1) = 무적** (사용자 결정, 처음엔 F3 따로였다가 F1로 합침): `Combat::SetPlayerInvincible` → 플레이어는 근접·탄 모두 안 맞음 (탄은 지나감). 판정 3개(죽는 중·대시 무적·디버그 무적)를 `Combat::CanBeHit` 한 곳으로. HUD에 `DEBUG: INVINCIBLE (F1)`
- **F4 = 한 번에 100마리** (F2는 1마리, 꾹 = 키 반복)
- **World 용량 256 → 9999** (사용자 결정, `World_U.h` 상수 1줄. 런타임 전환(평소 256 / 디버그 9999)도 검토했으나 `constexpr`이라 World 구조 변경이 필요 → 고정으로). 평소 게임은 수십 개라 상한만 올라감. 비용: `ForEachAlive`가 빈 칸까지 9999칸을 훑음 → Spatial Hash와 함께 측정 거리 (개선 아이디어: 사용 중인 최대 칸 번호까지만 순회)
- 테스트: 풀이 꽉 찰 때까지 소환·꽉 찬 뒤 투사체/드랍 생략(throw 없음)·무적 on/off

## 4. 맵 2차 축소 (11/19)
- 사용자: 더 작게 + 위로 올라가는 발판 + 위쪽에도 적
- 방 1: 72 → **56 x 22**, 방 2: 88 x 26 → **64 x 26** (세로 탑 3개). 발판 높이 차이 3~4칸 (점프 최고 약 5칸)
- 적 스폰에 `iStandTileY` 추가 (0 = 바닥, 아니면 그 발판 윗면) → 방 1: 8마리 중 6마리, 방 2: 11마리 중 8마리가 발판 위
- `RoomsDataTest_C`: 모든 방의 적이 벽과 안 겹치고 발 아래가 단단한지, 플레이어 시작 자리·문이 막히지 않는지 검사
- 측정 벤치는 튜토리얼 방이라 영향 없음
