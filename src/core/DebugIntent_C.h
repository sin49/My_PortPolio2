#pragma once

// 디버그 키 의도. Input이 기록, Game이 소비.
// F1 = 디버그 모드 (패널 + 플레이어 무적), F2 = 적 1마리 (꾹 = 키 반복), F4 = 적 kStressBurstCount마리, F3 = 검색 방식 전환, F5 = 측정 기록
struct DebugIntent
{
    bool bToggleDebugPressed = false;  // F1
    int iSpawnEnemyPresses = 0;        // F2 누른 횟수. 첫 틱이 소비
    int iSpawnBurstPresses = 0;        // F4 누른 횟수. 첫 틱이 소비
    bool bRecordPressed = false;       // F5 측정값 CSV 기록
    bool bToggleSpatialPressed = false; // F3 검색 방식 전환 (전체 순회 <-> 해시, 같은 장면 비교)
};

inline constexpr int kStressBurstCount = 100;
