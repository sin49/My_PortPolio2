#pragma once

// 물리 공용 값 (플레이어·적 공통)
struct PhysicsDef
{
    float fGravity = 2400.0f;       // px/s²
    float fMaxFallSpeed = 900.0f;   // px/s. 틱당 15px < 타일 32px -> 벽 뚫기 없음 (Physics_C.cpp static_assert)
};

inline constexpr PhysicsDef kPhysicsDef{};
