# 코드 가이드 (프로젝트 내부 설명서)

> 어디에 뭐가 있고, 뭘 바꾸려면 어디를 고치면 되는지. 규칙은 `CLAUDE.md`, 기획은 `docs/GDD.md`.

## 1. 빌드 · 실행 · 테스트
**Developer PowerShell for VS 2022**에서 프로젝트 폴더로 이동 후:
```
cmake --build build --config Debug --parallel; if ($?) { .\build\bin\Debug\Main.exe }   # 빌드 + 실행
ctest --test-dir build -C Debug --output-on-failure                                    # 테스트
.\build\bin\Debug\tests.exe                                                            # 테스트 (케이스별 결과)
```
- 평소엔 `cmake --build`만 하면 됨. CMakeLists.txt가 바뀌면 빌드 때 자동 재구성.
- **새 `.cpp`를 만들었을 때만** `cmake -S . -B build` (CMakeLists에 등록도 필요. World.cpp는 자동 등록)
- `build` 폴더를 지우면 처음 구성(약 70초, raylib 다운로드)부터 다시 함.
- Visual Studio로 열기: `build\Main.sln` → F5 (디버거·ASan 오류 위치 확인에 좋음)
- 손맛만 볼 땐 Release가 가벼움: `cmake --build build --config Release` → `build\bin\Release\Main.exe`

## 2. 조작
| 키 | 동작 |
|---|---|
| ← → | 이동 (관성) |
| Z | 점프 (길게 = 높이, 코요테·버퍼 있음, 달리면서 = 더 높이) |
| Shift 짧게 눌렀다 떼기 | 대시: 짧고 빠른 수평 돌진 + 0.1초 무적, 게이지 50% (`fDashTapTime` 0.18초 안에 떼야 함) |
| Shift 길게 누르고 있기 | 달리기 (방향키 있을 때만, 게이지 천천히 소모, 점프 더 높이) |
| X | 근접 공격 (버퍼 있음) |
| Enter | 시작 / 재시작 |
| F1 | 디버그 수치 켜기·끄기 |
| Esc | 종료 |

화면 표시: 노란 사각형 = 플레이어(흰 테두리 = 땅, 하늘색 = 달리기, 밝은 하늘색 = 대시, 반투명 = 무적), 빨간 반투명 = 공격 히트박스.
적 = 붉은 사각형, 크기 = 맞아야 죽는 횟수(작음 1 / 중간 2 / 큼 3), 머리 위 점 = 남은 타격 수.
왼쪽 위 = HP 바 + 대시 게이지(흰 눈금 = 대시 가능선 50%). 오른쪽 위 = 방 진행 점(마름모 = 보스) + 남은 적 칸.
남은 적이 처음의 1/4 이하 = 화면 밖 적 방향 화살표. 문 = 빨강(잠김)/초록(열림, 화면 오른쪽 화살표).
오른쪽 위 = 미니맵(그 아래 방 진행 점·남은 적 칸) (노란 점 = 나, 빨간 점 = 적, 흰 테두리 = 지금 화면). 플레이어 머리 위 = 대시 게이지(쓸 때만). 타이틀 = 조작법.
붉은 사각형 = 적(허수아비, 머리 위 HP 바), 맞으면 흰색 깜빡임 + 하얀 파편.

## 3. 수치 조정 (코드 로직은 안 건드려도 됨)
| 바꾸고 싶은 것 | 파일 | 값 |
|---|---|---|
| 걷기 속도 | `src/data/PlayerDef_C.h` | `fMoveSpeed` |
| 출발할 때 붙는 시간 / 멈출 때 미끄러짐 | `src/data/PlayerDef_C.h` | `fGroundAccelTime`, `fGroundStopTime` (공중: `fAir~`) |
| 달리기 배율·출발 속도·점프 배율 | `src/data/PlayerDef_C.h` | `fRunSpeedMultiplier`, `fDashAccelTime`, `fRunJumpMultiplier` |
| 대시 속도·시간·비용·무적 | `src/data/PlayerDef_C.h` | `fDashSpeed`, `fDashTime`, `fDashCost`, `fDashInvulnTime` |
| 게이지 (달리기 소모·회복·탈진 해제) | `src/data/PlayerDef_C.h` | `fRunDrainPerSec`, `fDashRegenPerSec`, `fDashRecoverRatio` |
| 적 크기별 몸집·HP | `src/data/EnemyDef_C.h` | `kEnemySizes` |
| 방 모양·적 배치(크기 포함)·튜토리얼 키 안내 | `src/data/Rooms_C.h` | `kRooms` (크기 + 벽 사각형 목록) |
| 카메라 따라가는 속도 | `src/render/ViewCamera_C.h` | `kFollowSharpness` |
| 점프 높이 / 짧게 누를 때 높이 | `src/data/PlayerDef_C.h` | `fJumpSpeed`, `fJumpCutMultiplier` |
| 코요테·점프 버퍼 시간 | `src/data/PlayerDef_C.h` | `fCoyoteTime`, `fJumpBufferTime` |
| 공격 범위·지속·대기·버퍼 | `src/data/PlayerDef_C.h` | `vAttackSize`, `fAttackActiveTime`, `fAttackRecoverTime`, `fAttackBufferTime` |
| 플레이어 크기 | `src/data/PlayerDef_C.h` | `vSize` |
| 플레이어 HP·공격력·넉백·깜빡임 | `src/data/PlayerDef_C.h` | `fMaxHp`, `fAttackDamage`, `fKnockbackSpeed/Up`, `fHitFlashTime` |
| 적 크기·HP·속도·공격력 | `src/data/EnemyDef_C.h` | `kChaserDef` |
| 적 배치 (테스트 방) | `src/data/TestRoom_C.h` | `kTestRoomEnemySpawns` |
| 피격 파편 개수·퍼짐·속도·수명 | `src/render/Particles_C.cpp` | 위쪽 `k~` 상수 |
| 중력·최대 낙하 속도 | `src/data/PhysicsDef_C.h` | `fGravity`, `fMaxFallSpeed` (낙하 속도는 틱당 32px 미만이어야 함, 넘으면 컴파일 에러로 알려줌) |
| 테스트 방 지형·스폰 위치 | `src/data/TestRoom_C.h` | `#` 벽 / `.` 빈칸, `kTestRoomPlayerSpawn` |
| 화면 색 | `src/render/Render_C.cpp` | 위쪽 `k~Color` 상수 |

수치를 바꾸면 테스트가 깨질 수 있음 (예: 점프 높이 테스트). 깨지면 의도한 변경인지 확인 후 테스트 기준을 같이 조정.

## 4. 한 프레임 흐름
```
렌더 프레임마다:  Input.Poll            키 -> MoveIntent / AttackIntent (눌린 순간은 래치)
고정 틱(1/60s)마다: PlayerMotor.Update  MoveIntent -> 속도 (관성·대시·점프 보조·facing)
                   Physics.Update      중력, 이동, 타일 충돌, 접지
                   SpatialQuery.Rebuild  (MVP: 할 일 없음, 해시 때 격자 재구축)
                   Combat.Update...    AttackIntent -> 공격 단계·히트박스
                   Combat.ResolveHits  겹침 -> HP·넉백·깜빡임, HP 0 적 삭제 예약 -> HitReport
                   Particles           HitReport -> 파편 생성·이동
                   World.FlushRemovals 삭제 예약 실제 처리
렌더 프레임마다:  Render.Draw          const&로 읽어서 그리기만
```
- 로직은 `GameLogic` 라이브러리(raylib 링크 안 됨). raylib을 include하면 빌드가 깨지는 게 정상 = 로직/렌더 분리 강제.

## 4-1. 구조 파악용: 읽는 순서 (30분 코스)
위에서 아래로. 각 파일은 짧음(대부분 100줄 이하).
| 순서 | 파일 | 볼 것 |
|---|---|---|
| 1 | `src/main_C.cpp` | 창 만들고 `Game` 하나 만들어 `Run()`. 프로그램 전체가 이게 끝 |
| 2 | `src/core/Game_C.h` | Game이 **소유하는 것 목록** = 이 게임의 전부 (시스템들 + World + 방) |
| 3 | `src/core/Game_C.cpp` | `Run()` = Unity의 Update+Render, `FixedUpdate()` = 시스템 호출 **순서**만 있음 |
| 4 | `src/core/Entity_U.h` | 엔티티 하나가 들고 있는 데이터 (몸·HP·의도·이동 상태·공격 상태·피격·AI) |
| 5 | `src/systems/Input_C.cpp` | 키 → `MoveIntent`/`AttackIntent` (위치는 안 건드림) |
| 6 | `src/systems/PlayerMotor_C.cpp` | 의도 → 속도 (관성·대시·점프 보조) |
| 7 | `src/systems/Physics_C.cpp` | 속도 → 위치 + 타일 충돌 |
| 8 | `src/systems/Combat_C.cpp` | 공격 키 → 히트박스 → 맞은 대상 HP·넉백 |
| 9 | `src/render/Render_C.cpp` | 전부 `const`로 읽어서 사각형 그리기 |

**예: X를 눌렀을 때 한 바퀴**
```
[렌더 프레임] Input.Poll        X 눌림 -> player.tAttackIntent.bAttackPressed = true (래치)
[고정 틱]     Motor·Physics     (공격과 무관, 평소대로 이동)
              Combat.UpdateAttack  bAttackPressed -> 버퍼 -> Ready면 Active 시작, 히트박스 = 몸 앞 40x32
              Combat.ResolveHits   SpatialQuery로 히트박스와 겹친 엔티티 -> 적이면 HP -10, 넉백, 깜빡임, HitReport
              Particles            HitReport -> 파편 생성
              World.FlushRemovals  HP 0 적 실제 삭제
              (Game)               래치 해제: bAttackPressed = false
[렌더 프레임] Render.Draw        히트박스(빨강)·깜빡임(흰색)·파편 그리기
```
적(Chaser)도 똑같음. 다른 건 첫 줄뿐: Input 대신 **EnemyAI가 의도를 씀.**

## 5. 이름 규칙 요약 (상세: CLAUDE.md "명명")
- 파일: AI만 작성 = `이름_C.h/.cpp`, 사용자만 작성 = `이름_U.h/.cpp`, 섞이면 접미사 없음 + 파일 안 `// work: claude` / `// work: user` 구역 주석
- 변수: 헝가리안 (`fSpeed`, `bGrounded`, `vPos`, `tBody`, 클래스 멤버는 `m_` 추가)
- **사용자는 신경 안 써도 됨.** 접미사 없이·아무 변수명으로 작성 → AI가 리뷰 때 맞춤

## 6. 파일 지도 (작성자 구분)
| 파일 | 책임 | 작성 |
|---|---|---|
| `src/main_C.cpp` | 창 생성, Game 실행 | AI |
| `src/core/Game_C.h/.cpp` | 시스템·데이터 소유, 호출 순서 | AI |
| `src/core/FixedTimestep_C.h` | 1/60s 누적기 | AI |
| `src/core/Vec2_C.h`, `Rect_C.h`, `Body_C.h` | 공용 타입 | AI |
| `src/core/MoveIntent_C.h`, `AttackIntent_C.h` | 의도 (Input·AI가 기록) | AI |
| `src/core/Tilemap_C.h/.cpp` | 타일 격자 | AI |
| `src/core/Handle_U.h`, `Entity_U.h`, `World_U.h/.cpp` | 엔티티 핸들 풀 | **사용자** (Codex 보조. 파일 분리·명명은 AI가 기계적으로 정리) |
| `src/systems/Input_C.h/.cpp` | 키 -> 의도 | AI |
| `src/systems/PlayerMotor_C.h/.cpp` | 의도 -> 속도 | AI |
| `src/systems/Physics_C.h/.cpp` | 중력·충돌 | AI |
| `src/systems/Combat_C.h/.cpp` | 근접 공격 단계, 피격 판정(HP·넉백·깜빡임) | AI |
| `src/systems/SpatialQuery_C.h/.cpp` | 주변 엔티티 검색 (MVP 전체 순회) | AI |
| `src/render/Particles_C.h/.cpp` | 피격 파편 (연출 전용) | AI |
| `src/systems/RoomFlow_C.h/.cpp` | 타이틀·방 진행·문·게임 오버·승리 | AI |
| `src/render/ViewCamera_C.h`, `Hud_C.h/.cpp` | 카메라(플레이어 따라감), 화면 UI | AI |
| `src/data/Rooms_C.h`, `EntityFactory_C.h` | 방 4개 데이터, 종류별 초기 Entity | AI |
| `src/systems/EnemyAI_U.h/.cpp` | 적 FSM (의도만 씀) | **사용자** |
| `src/systems/Stats_U.h/.cpp` | 스탯 계산 (기본값 + modifier, 원본 불변) | **사용자** (범위 for·마지막 순회 예시는 Codex 도움) |
| `src/systems/Projectile_C.h` | 투사체 상태 (`Faction`, `ProjectileState`) | AI |
| `src/systems/Items_C.h/.cpp` | 아이템 드랍·획득·스택 → modifier 목록 (스탯 직접 수정 X) | AI |
| `src/data/ItemDefs_C.h` | 아이템 테이블 5종, 플레이어 기본 스탯, 회복약 확률 | AI |
| `src/render/Render_C.h/.cpp` | 그리기 | AI |
| `src/data/*_C.h` | 튜닝 값·방 데이터 | AI (값 조정은 사용자) |
| `tests/*_C.cpp` | 테스트 (`WorldTest_C.cpp`는 World 스펙) | AI |

## 7. 문서
| 문서 | 내용 |
|---|---|
| `docs/GDD.md` | 기획 기준 (책임표 7장) |
| `docs/design/*.md` | 기능별 의사코드·결정 기록 (구현 전에 합의) |
| `docs/devlog.md` | 날짜별 한 일 / AI 제안 / 사용자 수정·거절 |
| `docs/cpp-notes.md` | 헷갈린 C++ 개념 (C# 비교) |
| `docs/gpt_support.md` | Codex/GPT 보조 역할·리뷰 메모·대행 인수인계 (Codex 관리) |
| `CLAUDE.md` / `AGENTS.md` | Claude / Codex가 따르는 프로젝트 규칙 |
