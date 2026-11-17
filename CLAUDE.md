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
- 멤버 변수는 **선언과 동시에 초기화** (`float m_fHp = 100.f;`).
- 종류 구분은 `enum class`. 문자열·숫자 리터럴로 타입 판별 금지.
- 다형성 기반 클래스엔 `virtual ~Base() = default;`, 파생은 `override` 명시.
- 코루틴 대신 **타이머 + 상태 enum**.
- 쓰지 않는 것: 템플릿 메타프로그래밍, 매크로 로직, 예외 기반 흐름 제어, 복잡한 상속 계층(2단계 초과).
  - 예외: **복구하지 않는 치명적 오류**에는 `throw` 허용 (예: World 용량 초과). `catch`해서 흐름을 이어가는 용도로는 쓰지 않음.

## 명명 (11/17 사용자 결정, 세부 규칙은 AI가 정함)
> **명명 처리는 AI 담당.** 사용자는 규칙을 신경 쓰지 않고 작성해도 됨. AI가 프로젝트 점검(리뷰) 때 변수명·파일명·작성자 주석을 맞춤.

- 타입·함수: `PascalCase` / 상수(`constexpr`, `const` 전역): `kPascalCase` / enum 값: `PascalCase`
- **변수: 헝가리안 표기** = 타입 접두사 + 이름(첫 글자 대문자). 지역 변수·파라미터·필드 모두 적용.
  | 타입 | 접두사 | 예 |
  |---|---|---|
  | `bool` | `b` | `bGrounded` |
  | `int` | `i` | `iFacing` |
  | `long long` | `ll` | `llTotalTicks` |
  | 부호 없는 정수 (`std::uint32_t`, `size_t`) | `u` | `uIndex` |
  | `float` / `double` | `f` / `d` | `fGravity` |
  | `char` | `c` | `cTile` |
  | `Vec2` | `v` | `vPos` |
  | `Rect` | `rc` | `rcHitbox` |
  | `Handle` | `h` | `hPlayer` |
  | `enum class` 값 | `e` | `ePhase` |
  | 그 밖의 구조체·클래스 (참조 포함) | `t` | `tBody`, `tWorld` |
  | 원시 포인터 | `p` (가리키는 타입 접두사는 안 붙임) | `pEntity` |
  | `std::vector` / `std::array` / `std::span` | `vec` / `arr` / `sp` | `vecHandles` |
  | `std::string` / `std::string_view` | `str` / `sv` | `svRow` |
  | `std::optional` / `std::unique_ptr` | `opt` / `up` | `optRoom` |
  | 람다·함수 객체 | `fn` | `fnVisit` |
- 범위 접두사: **클래스**(private 상태를 가진 것) 멤버 = `m_` + 타입 접두사 (`m_fAccumulator`) / 정적 멤버 = `s_`.
  **데이터 구조체**(public 필드, 지정 초기화로 만드는 것) 필드는 `m_` 없이 타입 접두사만 (`tBody.vPos.fX`).
- 예외: 짧은 루프 인덱스 `i`, `j`는 그대로 허용.
- **파일명 = 타입명 + 작성자 접미사** (포폴에서 AI/사용자 작성 범위를 보여주기 위함)
  - AI만 작성한 파일: `Name_C.h` / `Name_C.cpp`
  - 사용자만 작성한 파일: `Name_U.h` / `Name_U.cpp`
  - 둘이 섞인 파일: 접미사 없이 `Name.h` / `Name.cpp`. 대신 파일 안에서 구역마다 작성자 주석
    ```cpp
    // work: claude
    ...AI가 쓴 부분...
    // work: user
    ...사용자가 쓴 부분...
    ```
  - 작성자 구분은 `docs/code-guide.md`의 파일 표에도 기록.
  - **기계적인 이름 정리(헝가리안·접미사·include 수정)는 작성자를 바꾸지 않음.** 사용자 코드를 AI가 이름만 고쳐도 `_U` 유지.
- 조회 함수(`Get~`, `Is~`)는 상태를 바꾸지 않음.

## 협업 (Claude + Codex/GPT)
- 역할: Claude = 주 개발 / Codex = 리뷰·조언·C++ 학습 보조 / 사용자 = 최종 결정·World 직접 구현.
- Codex 규칙과 인수인계는 `docs/gpt_support.md` (Codex가 관리하는 문서. **Claude는 읽기만**, 수정하지 않음).
- Codex의 평소 수정 범위는 `docs/gpt_support.md`뿐. 사용자가 명시적으로 요청할 때만 Claude 업무를 대행함.
- **Claude는 작업 시작 시 `docs/gpt_support.md`의 "인수인계 기록"을 확인**하고, 대행 기록이 있으면 변경 파일·테스트 결과를 검증한 뒤 이어받음.
- Codex의 리뷰 의견은 확정 설계가 아님. 반영 여부는 사용자와 합의.

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
