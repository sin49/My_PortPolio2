#pragma once

struct Body;
class Tilemap;

// 중력 + 이동 + 타일 충돌 + 접지 판정 (플레이어·적 공용).
// 입력 의도는 모름. 속도는 Motor가 정해 둔 값을 그대로 적용.
class Physics
{
public:
    void Update(Body& tBody, const Tilemap& tMap, float fDt) const;
};
