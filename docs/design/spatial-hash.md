# Spatial Hash (주변 검색 최적화) — 설계 (**사용자 구현**)

> 상태: **구현 완료 (11/19)** — 사용자 `SpatialHash_U` (스펙 7개 통과) + AI `SpatialQuery` 연결 (결과 동등성 테스트, F3·`--full-scan` 전환). 측정은 measure.md
> 기준: CLAUDE.md "주변 엔티티 검색은 반드시 `SpatialQuery::QueryNearby(rect)`. MVP는 전체 순회, 이후 Spatial Hash로 교체해도 호출부는 바뀌지 않아야 함"

## 0. 무엇을, 왜
- 지금 `QueryNearby`는 **World 전체를 매번 순회**. Combat이 휘두를 때마다, 투사체 1발마다 부름 → 엔티티 N개, 질문 M번이면 비교 N × M.
  - 투사체가 생기면 M이 커짐 (보스 원형탄 8발 × 매 틱). 방 2는 120 × 30칸으로 넓어서 대부분 멀리 있는데도 전부 비교.
- **Spatial Hash** = 방을 같은 크기 칸(예: 128px)으로 나누고, 각 칸에 "이 칸에 걸친 엔티티 핸들" 목록을 둠.
  질문 사각형이 걸치는 칸만 보면 됨 → 비교 수가 "근처에 있는 것"만큼으로 줄어듦.
- 포폴에 쓸 말: **같은 인터페이스(`SpatialQuery`) 뒤에서 내부만 교체** + **비교 횟수를 숫자로 측정** (F1 디버그 패널에 AI가 표시).

## 1. 역할 나눔
| 누가 | 파일 | 하는 일 |
|---|---|---|
| **사용자** | `src/systems/SpatialHash.h` / `.cpp` | 순수 격자 자료구조: `Clear`, `Insert(핸들, 사각형)`, `Query(사각형) -> 후보 핸들`. **World도 Entity도 모름** → 테스트하기 제일 쉬움 |
| AI | `src/systems/SpatialQuery_C` | `Rebuild()`에서 World를 한 번 돌며 `Insert`, `QueryNearby()`는 `Query` 후보 중 **정확히 겹치는 것만** 남김 + 비교 횟수 측정. 호출부(Combat·투사체·Items)는 그대로 |
| AI | `tests/SpatialHashTest_C.cpp` | 스펙 테스트 7개 (아래 4장). **전부 통과 = 완료** |

- 파일을 만든 뒤 `cmake -S . -B build` 한 번 → 출력 `SpatialHash 구현 포함: ON` → 스펙 테스트도 같이 돎.
- 이름(`SpatialHash`, `Clear`, `Insert`, `Query`, `kDefaultCellSize`)은 테스트가 쓰므로 그대로. 나머지는 자유 (리뷰 때 AI가 정리).
- **raylib include 금지** (GameLogic = 테스트 가능한 로직).

## 2. SpatialHash.h
```
#pragma once
#include "core/Handle_U.h"
#include "core/Rect_C.h"
#include <cstdint>                       // std::int64_t
#include <unordered_map>                 // [C++] C#의 Dictionary<K, V>
#include <vector>

class SpatialHash {
public:
    static constexpr float kDefaultCellSize = 128.0f;     // 4타일. 제일 큰 적(38x50)·보스(64x72)가 칸 1~4개에 걸침

    explicit SpatialHash(float fCellSize = kDefaultCellSize)
        // [C++] explicit = SpatialHash h = 64.0f; 같은 "몰래 변환"을 막음. 생성자 인자 1개면 붙이는 습관
        // [C++] 기본 인자(= kDefaultCellSize)는 h에만 씀. cpp에 또 쓰면 컴파일 에러

    void Clear()                                           // 매 틱 Rebuild 시작에 호출
    void Insert(Handle hEntity, const Rect& rcBounds)      // rcBounds가 걸치는 모든 칸에 hEntity 추가
    void Query(const Rect& rcArea, std::vector<Handle>& vecOut) const
        // rcArea가 걸치는 칸들에 든 핸들 = "후보". 정확한 겹침 검사는 안 해도 됨 (SpatialQuery가 함)
        // 단, 같은 핸들은 한 번만 (엔티티가 칸 여러 개에 걸치면 여러 칸에 들어 있으니까)
        // vecOut은 처음에 비우고 채움 (호출자 버퍼 재사용 = 할당 없음)

private:
    // 칸 좌표 (iCellX, iCellY) -> 키 하나로. 칸 좌표는 음수도 가능 (방 밖으로 살짝 나간 몸)
    static std::int64_t MakeKey(int iCellX, int iCellY)
    int ToCell(float fCoord) const                          // 픽셀 좌표 -> 칸 번호

    float m_fCellSize = kDefaultCellSize
    std::unordered_map<std::int64_t, std::vector<Handle>> m_mapCells
}
```

## 3. SpatialHash.cpp
```
#include "systems/SpatialHash.h"
#include <algorithm>                     // std::sort, std::unique
#include <cmath>                         // std::floor

SpatialHash::SpatialHash(float fCellSize) : m_fCellSize(fCellSize) {}
    // [C++] ": m_fCellSize(fCellSize)" = 멤버 초기화 목록. 본문 { } 전에 멤버를 바로 그 값으로 만듦 (C#엔 없음)

ToCell(fCoord) -> int:
    return static_cast<int>(std::floor(fCoord / m_fCellSize))
    // [C++] 그냥 (int)(-10 / 128.0f) 는 0이 됨 (0 쪽으로 자름). floor를 써야 -1 → 음수 좌표가 옆 칸과 안 섞임

MakeKey(iCellX, iCellY) -> std::int64_t:
    return (static_cast<std::int64_t>(iCellX) << 32) | static_cast<stduint32_::t>(iCellY)
    // 위 32비트 = x, 아래 32비트 = y. C#의 ((long)x << 32) | (uint)y 와 같음
    // [C++] iCellY를 uint32로 먼저 바꾸는 이유: 음수 int를 바로 int64로 늘리면 위 32비트가 전부 1로 채워져 x를 덮어씀

Clear():
    for (auto& [llKeHandles] : y, vecm_mapCells):   // [C++] 구조 분해 = C#의 foreach (var (key, list) in dict)
        vecHandles.clear()
    // ★ m_mapCells.clear()를 쓰지 말 것: 칸(vector)까지 지워져서 매 틱 메모리를 다시 할당함.
    //   vector.clear()는 내용만 비우고 용량은 남김 → 몇 틱 뒤로는 할당 0 (World 풀과 같은 생각)
    //   (리뷰 포인트: 방이 바뀌면 안 쓰는 칸이 남지만 수백 개 수준이라 무시. 원하면 방 전환 때 m_mapCells.clear() 1번)

Insert(hEntity, rcBounds):
    iMinX = ToCell(rcBounds.fX),               iMaxX = ToCell(rcBounds.fX + rcBounds.fW)
    iMinY = ToCell(rcBounds.fY),               iMaxY = ToCell(rcBounds.fY + rcBounds.fH)
    for iY in iMinY..iMaxY (끝 포함):
        for iX in iMinX..iMaxX (끝 포함):
            m_mapCells[MakeKey(iX, iY)].push_back(hEntity)
            // [C++] map[key]는 없으면 빈 vector를 새로 만들어 넣고 그걸 돌려줌 (C# Dictionary는 예외 던짐)

Query(rcArea, vecOut) const:
    vecOut.clear()
    (Insert와 같은 방법으로 iMinX..iMaxX, iMinY..iMaxY 계산)
    for 각 칸 (iX, iY):
        auto it = m_mapCells.find(MakeKey(iX, iY))
            // [C++] const 함수라 map[key] 못 씀 (없으면 새로 넣는 함수라서). find + end() 비교
        if it == m_mapCells.end(): continue
        vecOut.insert(vecOut.end(), it->second.begin(), it->second.end())   // 그 칸의 핸들 전부 뒤에 붙임
            // [C++] it->first = 키, it->second = 값 (vector)
    // 중복 제거: 정렬 -> 붙어 있는 같은 값 제거
    std::sort(vecOut.begin(), vecOut.end(), [](Handle a, Handle b) { return a.uIndex < b.uIndex })
    vecOut.erase(std::unique(vecOut.begin(), vecOut.end()), vecOut.end())
        // [C++] std::unique는 지우지 않고 "남길 것"을 앞으로 모은 뒤 새 끝을 돌려줌 → erase로 실제로 잘라야 함
        //       (Handle에 operator==가 이미 있어서 unique가 바로 됨)
```

## 4. 스펙 테스트 (AI, `tests/SpatialHashTest_C.cpp`)
1. 비어 있으면 결과 없음
2. 넣은 것과 겹치는 영역 → 나옴 / 먼 영역 → 안 나옴
3. 칸 4개에 걸친 엔티티 → **1번만** 나옴 (중복 제거)
4. `Clear` 뒤엔 안 나옴, 다시 `Insert`하면 나옴 (재사용)
5. 음수 좌표 (방 밖으로 살짝 나간 몸) → 0번 칸과 섞이지 않음
6. 랜덤 사각형 300개 × 질문 100번: **전체 순회로 찾은 겹침은 전부 후보에 들어 있음** (빠뜨리면 안 됨. 후보가 더 많은 건 OK)
7. 칸 크기를 바꿔도(32·128·512) 6번이 성립

## 5. AI 작업 (사용자 구현 후)
- `SpatialQuery_C`: `Rebuild()` = `Clear` + World 순회 `Insert`, `QueryNearby` = `Query` 후보 → `Get`으로 꺼내 `Overlaps` 검사
- 측정: 틱당 겹침 비교 횟수 (해시 vs 전체 순회)를 F1 패널에 표시 + 스트레스 모드 (F2: 적 150마리 소환) → 수치를 README에
