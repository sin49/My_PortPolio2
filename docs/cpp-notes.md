# C++ 노트 (C# 개발자 관점)

헷갈렸던 C++ 개념을 한 항목씩 C# vs C++ 형태로 정리함.

---

## 표준 라이브러리는 쓰는 헤더를 직접 include
- **언제 헷갈렸나:** 2026-11-22 / SSAFY 코딩 시험. `string`을 쓰려면 헤더가 필요한 걸 몰라서 문자열 풀이를 못 씀
- **C#:**
  ```csharp
  using System;            // 네임스페이스만 열면 끝. string은 언어 기본 타입
  string s = x.ToString();
  ```
- **C++:**
  ```cpp
  #include <string>        // std::string, std::to_string, std::stoll
  #include <vector>        // std::vector
  #include <algorithm>     // std::sort, std::remove
  std::string s = std::to_string(x);
  ```
- **차이 핵심:** C#의 `using`은 이름만 열지만, C++의 `#include`는 그 기능의 선언 자체를 가져옴. 안 넣으면 "식별자를 찾을 수 없음" 컴파일 에러.
  `<iostream>`만 넣어도 `string`이 될 때가 있지만 컴파일러마다 달라서 믿으면 안 됨 (`to_string`·`stoll`은 특히 안 될 수 있음).
- **시험용 메모:** 헤더가 기억 안 나면 문자열 없이 `% 10`, `/ 10`으로 자릿수를 다루면 됨 (`<iostream>`만으로 충분).
- **이 프로젝트에서의 규칙:** 쓰는 헤더는 그 파일에서 직접 include (다른 헤더가 대신 가져오는 것에 기대지 않기).

---

<!-- 템플릿: 아래 블록을 복사해서 사용

## 개념 이름
- **언제 헷갈렸나:** (날짜 / 어떤 코드에서)
- **C#:**
  ```csharp
  ```
- **C++:**
  ```cpp
  ```
- **차이 핵심:** (1~3줄)
- **이 프로젝트에서의 규칙:** (CLAUDE.md 관련 항목)

-->
