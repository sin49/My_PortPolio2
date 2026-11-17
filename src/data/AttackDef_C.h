#pragma once

#include "core/Vec2_C.h"

// 공격 한 종류의 데이터 (플레이어·적 공용). 나중에 조합 키 공격(돌진 공격 등)도 이걸 하나 더 만들면 됨.
struct AttackDef
{
    Vec2 vSize = {};            // 히트박스 크기. 몸 앞쪽, 세로는 몸 가운데
    float fActiveTime = 0.0f;   // 히트박스 유지
    float fRecoverTime = 0.0f;  // 다음 공격까지 대기
    float fBufferTime = 0.0f;   // 대기 중 미리 누른 공격 기억 (AI는 0)
};
