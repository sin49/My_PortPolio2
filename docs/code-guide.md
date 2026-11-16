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
| Z | 점프 (길게 = 높이, 코요테·버퍼 있음) |
| Shift | 대시 = 누르는 동안 빨리 달리기 (게이지 소모) |
| X | 근접 공격 (버퍼 있음) |
| Esc | 종료 |

화면 표시: 노란 사각형 = 플레이어(흰 테두리 = 땅에 서 있음, 하늘색 = 대시 중), 빨간 반투명 = 공격 히트박스, 왼쪽 위 = 디버그 수치.

## 3. 수치 조정 (코드 로직은 안 건드려도 됨)
| 바꾸고 싶은 것 | 파일 | 값 |
|---|---|---|
| 걷기 속도 | `src/data/PlayerDef_C.h` | `fMoveSpeed` |
| 출발할 때 붙는 시간 / 멈출 때 미끄러짐 | `src/data/PlayerDef_C.h` | `fGroundAccelTime`, `fGroundStopTime` (공중: `fAir~`) |
| 대시 배율·출발 속도 | `src/data/PlayerDef_C.h` | `fDashSpeedMultiplier`, `fDashAccelTime` |
| 대시 게이지 (지속·회복·탈진 해제) | `src/data/PlayerDef_C.h` | `fDashDrainPerSec`, `fDashRegenPerSec`, `fDashRecoverRatio` |
| 점프 높이 / 짧게 누를 때 높이 | `src/data/PlayerDef_C.h` | `fJumpSpeed`, `fJumpCutMultiplier` |
| 코요테·점프 버퍼 시간 | `src/data/PlayerDef_C.h` | `fCoyoteTime`, `fJumpBufferTime` |
| 공격 범위·지속·대기·버퍼 | `src/data/PlayerDef_C.h` | `vAttackSize`, `fAttackActiveTime`, `fAttackRecoverTime`, `fAttackBufferTime` |
| 플레이어 크기 | `src/data/PlayerDef_C.h` | `vSize` |
| 중력·최대 낙하 속도 | `src/data/PhysicsDef_C.h` | `fGravity`, `fMaxFallSpeed` (낙하 속도는 틱당 32px 미만이어야 함, 넘으면 컴파일 에러로 알려줌) |
| 테스트 방 지형·스폰 위치 | `src/data/TestRoom_C.h` | `#` 벽 / `.` 빈칸, `kTestRoomPlayerSpawn` |
| 화면 색 | `src/render/Render_C.cpp` | 위쪽 `k~Color` 상수 |

수치를 바꾸면 테스트가 깨질 수 있음 (예: 점프 높이 테스트). 깨지면 의도한 변경인지 확인 후 테스트 기준을 같이 조정.

## 4. 한 프레임 흐름
```
렌더 프레임마다:  Input.Poll            키 -> MoveIntent / AttackIntent (눌린 순간은 래치)
고정 틱(1/60s)마다: PlayerMotor.Update  MoveIntent -> 속도 (관성·대시·점프 보조·facing)
                   Physics.Update      중력, 이동, 타일 충돌, 접지
                   Combat.Update...    AttackIntent -> 공격 단계·히트박스
렌더 프레임마다:  Render.Draw          const&로 읽어서 그리기만
```
- 로직은 `GameLogic` 라이브러리(raylib 링크 안 됨). raylib을 include하면 빌드가 깨지는 게 정상 = 로직/렌더 분리 강제.

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
| `src/core/Handle`, `Entity`, `World` (.h/.cpp) | 엔티티 핸들 풀 | **사용자** (설계: `docs/design/world.md`, 리뷰 후 `_U`) |
| `src/systems/Input_C.h/.cpp` | 키 -> 의도 | AI |
| `src/systems/PlayerMotor_C.h/.cpp` | 의도 -> 속도 | AI |
| `src/systems/Physics_C.h/.cpp` | 중력·충돌 | AI |
| `src/systems/Combat_C.h/.cpp` | 근접 공격 | AI |
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
