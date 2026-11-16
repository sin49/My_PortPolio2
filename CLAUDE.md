# CLAUDE.md — 프로젝트 규칙

## 프로젝트
- C++20 + raylib 2D 룸 클리어 로그라이트 프로토타입. 기획은 `docs/GDD.md`가 기준.
- 개발자는 Unity/C# 주력, C++은 대학 수업 수준 경험. **C++ 문법 설명이 필요할 수 있음.**
- 포폴 목적: 책임 분리 설계 + C++ 수명 관리 + AI 협업 과정 기록.

## 빌드
- Windows, CMake + Visual Studio 2022. raylib는 `FetchContent`로 가져옴.
- 구성: `cmake -S . -B build` / 빌드: `cmake --build build --config Debug`
- Debug 빌드는 경고 최대(`/W4`) + AddressSanitizer(`/fsanitize=address`) 사용.
- 코드를 바꾸면 **반드시 빌드해서 경고/에러 없는 걸 확인**하고 보고할 것.
- 테스트: doctest(FetchContent), 타깃 `tests`. 실행 `ctest --test-dir build -C Debug`. 로직 변경 시 관련 테스트 통과 확인.
- 테스트 대상 로직(Stats, World 풀 등)은 raylib에 의존하지 않게 작성할 것.

## 핵심 구조 결정 (변경 시 먼저 논의)
- 메인 루프는 **고정 타임스텝** (1/60s 누적기). 게임 로직은 고정 dt만 받음. 렌더는 매 프레임.
- 엔티티는 `World`의 **고정 슬롯 풀 + 세대 번호 핸들**로 관리. 외부는 `Handle{index, gen}`만 보관, 접근은 `Get(h)`(무효면 nullptr).
- 적 AI는 **명시적 FSM** (`enum class EnemyState` + 상태별 Enter/Update, 타이머 전이). 상태 시간은 데이터 테이블에서.
- 주변 엔티티 검색은 반드시 `SpatialQuery::QueryNearby(rect)`를 거침. Combat/AI에서 엔티티 목록을 직접 전체 순회하지 말 것.
  (MVP 구현은 전체 순회, 이후 Spatial Hash로 교체 예정. 교체해도 호출부는 바뀌지 않아야 함)
- 사용자가 직접 작성하기로 한 부분: `World` 핸들 풀 → 리뷰만 할 것.

## 작업 방식 (가장 중요)
1. 새 기능은 **코드 전에 의사코드부터** 제시하고 합의 후 구현함. 의사코드는 `docs/design/` 아래 `.md`로 남김.
2. 사용자가 C# 문법으로 의사코드를 써도 됨. C++로 옮길 때 C#과 다른 점을 **3줄 이내**로 설명함.
3. 사용자가 헷갈린 C++ 개념은 `docs/cpp-notes.md`에 C# vs C++ 형태로 한 항목씩 추가함.
4. 한 번에 한 기능. 기능 하나 끝나면 사용자가 실행 확인 → 커밋.
5. 사용자가 직접 짜겠다고 한 부분은 대신 짜지 말고 리뷰만 함.
6. 설계 문서(`docs/GDD.md`)와 다른 결정을 하게 되면 먼저 말하고 문서도 같이 고침.

## 책임 분리 규칙
- 모듈 책임표는 `docs/GDD.md` 7장을 따름.
- **매니저/싱글톤/전역 변수 금지.** `Game`이 시스템과 `World`를 소유하고 필요한 것만 참조로 넘김.
- `Render`는 게임 상태를 **읽기만** 함. `const&`로 받음.
- AI와 Input은 **의도만 기록**하고 위치/HP를 직접 바꾸지 않음.
- 시스템 간 직접 호출 대신 `Events`로 알림. 구독은 해제 가능해야 함.
- 파일 하나가 300줄을 넘거나 책임이 2개 이상 보이면 분리를 제안할 것.
- 스탯은 **덮어쓰지 않음.** 기본값 + modifier 재계산만 허용.

## C++ 규칙
- 소유: 값 또는 `std::unique_ptr`. `new`/`delete` 직접 사용 금지. 빌려 쓸 땐 참조 또는 원시 포인터(소유 안 함).
- 엔티티를 오래 들고 있을 땐 포인터 말고 **ID/핸들**로 참조. (`std::vector` 재할당 시 포인터가 깨짐)
- **순회 중 컨테이너에서 삭제 금지.** 삭제는 예약 후 `World::FlushRemovals()`에서 일괄 처리.
- 멤버 변수는 **선언과 동시에 초기화** (`float hp = 100.f;`).
- 종류 구분은 `enum class`. 문자열·숫자 리터럴로 타입 판별 금지.
- 다형성 기반 클래스엔 `virtual ~Base() = default;`, 파생은 `override` 명시.
- 코루틴 대신 **타이머 + 상태 enum**.
- 쓰지 않는 것: 템플릿 메타프로그래밍, 매크로 로직, 예외 기반 흐름 제어, 복잡한 상속 계층(2단계 초과).

## 명명
- 타입·함수: `PascalCase` / 변수·파라미터: `camelCase` / 상수: `kPascalCase` / 파일: 타입명과 동일.
- 조회 함수(`Get~`, `Is~`)는 상태를 바꾸지 않음.

## 폴더
```
src/
  core/      Game, World, Events, 공용 타입(Vec, Rect, EntityId)
  systems/   Input, Physics, Combat, EnemyAI, Stats, Items, RoomFlow
  render/    Render, HUD
  data/      아이템·적·방 정의 테이블
assets/      rooms/*.txt (타일맵)
docs/        GDD.md, design/, cpp-notes.md, devlog.md
```

## 기록
- `docs/devlog.md`: 날짜별로 "한 일 / AI가 제안한 것 / 사용자가 수정·거절한 것과 이유"를 짧게 남김. (AI 협업 포폴 근거)
- 커밋 메시지: `[영역] 요약` (예: `[combat] 근접 히트박스 1회 판정`)
