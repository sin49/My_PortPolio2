#pragma once

#include "core/Vec2_C.h"

// 투사체 1종 통일 (GDD 범위: 원거리 공격은 투사체 1종). Shooter·보스가 같이 씀
struct ProjectileDef
{
    Vec2 vSize = {12.0f, 12.0f};
    float fSpeed = 320.0f;
    float fLifetime = 3.0f;  // 초. 안 맞으면 이 뒤에 사라짐
    float fDamage = 10.0f;
    float fKnockbackSpeed = 200.0f;  // 근접(320)보다 약하게
};

inline constexpr ProjectileDef kBulletDef{};
