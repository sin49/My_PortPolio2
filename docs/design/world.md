# World (엔티티 핸들 풀) — 설계

> 상태: **합의됨 (2026-11-17)**. 사용자가 구현 → `tests/WorldTest_C.cpp`(AI 작성 스펙)를 통과시키면 완료 → AI 리뷰.
> 기준: GDD 11장 핸들 풀 의사코드, CLAUDE.md "핵심 구조 결정"

## 1. 결정 기록
| # | 질문 | 결정 | 비고 |
|---|---|---|---|
| 1 | 엔티티 데이터 모양 | **`Entity` 하나에 다 넣음** | 덩치 커지면 종류별 풀로 분리 검토 |
| 2 | 플레이어도 World에 | **넣음** | Game은 `playerHandle`만 보관 |
| 3 | 용량 | **256** | 금요일 스트레스 모드 때 조정 |
| 4 | 꽉 찼을 때 `Create()` | **`throw std::runtime_error`** | 크기를 안 늘려야 "고정 슬롯"의 메모리 의미가 있음. 치명적 오류로만 쓰고 catch로 흐름 제어 안 함 (CLAUDE.md 예외 조항). TODO: 나중에 생성 대기 큐 |
| 5 | 무효 핸들 | **세대 번호 1부터, `Handle{}`(gen 0) = 무효** | |
| 6 | 순회 | **`ForEachAlive(함수)`** | 성능은 아래 3장. 구조 개선은 나중에 |
| 7 | 삭제 예약 후 `Get` | **A: Flush 전까지 유효** | 장단점은 아래 4장 |
| 8 | 두 번 `Destroy` | **무시** | |
| 9 | 테스트 | **AI가 스펙 테스트를 먼저 작성** | `tests/WorldTest_C.cpp` |

## 2. 인터페이스 (테스트가 이 모양을 기대함)
> **이 블록의 이름은 그대로 복사할 것** (테스트가 이 필드명으로 접근함. 헝가리안 표기, CLAUDE.md 명명 규칙).
> 그 밖의 내부 변수(Slot 필드, 지역 변수 등)는 아무 이름이나 써도 됨 → 리뷰 때 AI가 맞춤.
> 파일명도 접미사 없이 `World.h` 등으로 만들면 됨 → 리뷰 때 AI가 `_U`로 바꿈.
```cpp
// src/core/Handle.h
#include <cstdint>
struct Handle {
    std::uint32_t uIndex = 0;
    std::uint32_t uGen = 0;             // 0 = 무효 (살아 있는 칸은 항상 1 이상)
    bool IsNull() const;                // uGen == 0
    bool operator==(const Handle&) const = default;
};

// src/core/Entity.h
#include "core/Body_C.h"
enum class EntityType { Player, Enemy, Drop };
struct Entity {
    EntityType eType = EntityType::Enemy;
    Body tBody;
    float fHp = 0.0f;
    // 종류별 상태(PlayerMotorState, EnemyState 등)는 필요해질 때 추가
};

// src/core/World.h
class World {
public:
    static constexpr std::uint32_t kCapacity = 256;

    World();                                    // 칸 256개를 한 번에 만들어 둠

    Handle Create(const Entity& initial);       // 빈 칸이 없으면 throw std::runtime_error
    Entity* Get(Handle h);                      // 무효·죽은·세대 불일치·범위 밖 -> nullptr
    const Entity* Get(Handle h) const;          // const World에서도 읽기 가능 (Render용)
    void Destroy(Handle h);                     // 예약만. 무효·이미 예약된 핸들은 무시
    void FlushRemovals();                       // 예약된 칸 실제 해제
    int GetAliveCount() const;                  // 살아 있는(예약 포함) 엔티티 수

    template <typename Fn>
    void ForEachAlive(Fn&& fnVisit);            // fnVisit(Handle, Entity&) — 헤더에 구현 (아래 3장)
};
```
- `Fn&&`의 C# 비교: `Action<Handle, Entity>`를 받는 것과 같은 역할. 템플릿이라 호출이 인라인됨.

## 3. 의사코드
```
Slot { Entity value; uint32 gen = 1; bool alive = false; bool pendingDestroy = false; }

World():
    slots = Slot 256개 (std::vector를 생성자에서 한 번 resize, 이후 크기 변경 금지)
    freeList = 255, 254, ..., 0     // 뒤에서 pop -> 0번부터 쓰게
    pendingList = 비어 있음 (reserve(256))

Create(initial):
    if freeList 비었음: throw runtime_error("World 용량 초과")
    idx = freeList.pop_back()
    slots[idx].value = initial; alive = true; pendingDestroy = false
    return Handle{idx, slots[idx].gen}

Get(h):
    if h.index >= 256: return nullptr
    s = slots[h.index]
    return (s.alive && s.gen == h.gen) ? &s.value : nullptr     // gen 0 핸들은 항상 불일치

Destroy(h):
    if Get(h) == nullptr: return            // 무효 핸들 무시
    if slots[h.index].pendingDestroy: return // 중복 무시 (결정 8)
    slots[h.index].pendingDestroy = true
    pendingList.push_back(h.index)          // 실제 삭제는 Flush에서 (결정 7: 그때까지 Get 유효)

FlushRemovals():
    for idx in pendingList:
        s = slots[idx]
        s.alive = false; s.pendingDestroy = false
        s.gen += 1                           // 옛 핸들 전부 무효화의 핵심
        s.value = Entity{}                   // 남은 값 초기화 (선택)
        freeList.push_back(idx)
    pendingList.clear()

ForEachAlive(fn):
    for idx in 0..255:
        if slots[idx].alive: fn(Handle{idx, slots[idx].gen}, slots[idx].value)
    // 순회 중 Destroy는 예약만 하므로 안전. 순회 중 Create도 칸 재할당이 없어서 메모리는 안전
    // (단, 새 엔티티가 이번 순회에 포함될지는 칸 위치에 따라 다름 -> 의존하지 말 것)
```

### 3-1. ForEachAlive 성능 (질문 6)
- C++에서 람다를 **템플릿 파라미터**(`template <typename Fn>`)로 받으면 컴파일러가 호출을 인라인해서 **그냥 for문과 거의 같은 속도**.
  (C#에서 foreach가 느렸던 건 주로 옛 Mono의 열거자 GC 할당 때문. C++엔 GC가 없음)
- `std::function`으로 받으면 간접 호출이 생겨 느려짐 -> 템플릿으로 받을 것. 이건 메타프로그래밍이 아니라 평범한 함수 템플릿이라 CLAUDE.md 규칙에 걸리지 않음.
- 진짜 비용은 **죽은 칸까지 256개를 다 훑는 것**. 지금은 무시할 수준. 스트레스 모드에서 느리면 "살아 있는 인덱스 목록(dense list)" 추가로 개선.

## 4. 결정 7 — A(Flush 전까지 유효) vs B(Destroy 즉시 nullptr)
> Unity 비유: GC가 아니라 **`Destroy()`의 동작**과 같음. Unity도 `Destroy(obj)` 후 같은 프레임 동안은 접근되고, 프레임 끝에 실제로 사라짐 = **A**.

| | A: Flush 전까지 유효 (채택) | B: Destroy 즉시 nullptr |
|---|---|---|
| 장점 | 죽는 프레임에 위치·종류를 그대로 읽을 수 있음 (드랍 생성, 이펙트, 킬 이벤트 처리) / 한 프레임 안에서 모든 시스템이 같은 상태를 봄 | "이미 죽은 적"을 다른 시스템이 다시 처리할 일이 없음 / 판단이 단순 |
| 신경 쓸 점 | **같은 틱에 두 번 죽는 문제**: 한 틱에 두 공격이 같은 적을 맞히면 킬 이벤트·드랍이 2번 생길 수 있음 → HP가 이미 0 이하면 무시하거나, `IsPendingDestroy(h)` 질의를 추가 / AI·Combat이 죽어가는 적을 계속 움직이거나 공격시킬 수 있음 → 사망 표시(`hp <= 0`) 확인 | 죽은 엔티티 정보가 필요한 처리는 `Destroy` **전에** 값을 복사해 둬야 함 / 순회 중간에 같은 엔티티가 갑자기 nullptr이 되므로 시스템마다 null 체크 증가 |
| 메모리 안전 | 둘 다 안전. 칸은 Flush 전까지 재사용되지 않음 | |

## 5. 작업 위치 (사용자 구현)
| 파일 | 할 일 | 상태 |
|---|---|---|
| `src/core/Handle.h` | 새로 만들기. `Handle` 구조체 | 사용자 |
| `src/core/Entity.h` | 새로 만들기. `EntityType`, `Entity` | 사용자 |
| `src/core/World.h` | 새로 만들기. 클래스 선언 + `ForEachAlive` 템플릿 본문(템플릿은 헤더에 있어야 함) | 사용자 |
| `src/core/World.cpp` | 새로 만들기. 나머지 함수 구현 | 사용자 |
| (리뷰 후) | 위 4개를 `_U`로 이름 변경, 헝가리안 정리, include 수정 | AI |
| `CMakeLists.txt` | 수정 불필요. `World.cpp`가 생기면 자동 포함 → `cmake -S . -B build` 한 번 | 완료 (AI) |
| `tests/WorldTest_C.cpp` | 스펙 테스트. 수정하지 말고 통과시키기 (테스트가 틀렸다고 생각되면 알려주기) | 완료 (AI) |

확인 방법:
```
cmake -S . -B build                        # World.cpp 만든 직후 한 번 (출력에 "World 구현 포함: ON")
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

## 6. World 완성 후 (AI 작업, 리뷰 다음)
- `Game`의 플레이어를 World로 이전 (`playerHandle`), Render는 `const World&`로 읽기
- `SpatialQuery::QueryNearby(rect)` (MVP: World 전체 순회)
- Chaser 적 + FSM, Combat 히트 판정(`alreadyHit`에 Handle 저장), HP
