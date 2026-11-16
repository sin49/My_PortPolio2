#pragma once

#include "core/Vec2_C.h"

// 물리 몸체 (플레이어·적 공용). AABB 왼쪽 위가 vPos, y는 아래가 +.
// 수정 권한: vVel(이동 의도 반영) -> Motor, vPos/vVel/bGrounded -> Physics
struct Body
{
    Vec2 vPos = {};
    Vec2 vSize = {};
    Vec2 vVel = {};          // px/s
    bool bGrounded = false;  // 마지막 Physics 틱에서 바닥에 닿았는지
};
