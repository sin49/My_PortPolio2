#pragma once

#include "core/Rect_C.h"
#include "core/Vec2_C.h"

// 물리 몸체 (플레이어·적 공용). AABB 왼쪽 위가 vPos, y는 아래가 +.
// 수정 권한: vVel(이동 의도 반영) -> Motor, vPos/vVel/bGrounded -> Physics
struct Body
{
    Vec2 vPos = {};
    Vec2 vSize = {};
    Vec2 vVel = {};          // px/s
    bool bGrounded = false;  // 마지막 Physics 틱에서 바닥에 닿았는지
    bool bNoGravity = false; // 이번 틱 중력 끔 (대시 중). Motor가 매 틱 다시 정함
};

inline Rect GetBounds(const Body& tBody)
{
    return Rect{tBody.vPos.fX, tBody.vPos.fY, tBody.vSize.fX, tBody.vSize.fY};
}
