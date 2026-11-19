#pragma once
// 적이 발판 끝에서 떨어지지 않게 (11/19, 사용자: "발판 위 적이 바로 뛰어내려서 발판이 의미 없음").
// AI는 타일을 모름 -> AI가 쓴 이동 의도를 "발판 끝이면 0"으로 거르기만 함 (위치·HP는 안 바꿈).
// 순서: 적 AI들 -> LedgeGuard -> Motor. 사용자 EnemyAI는 수정하지 않음
class Tilemap;
class World;

class LedgeGuard
{
public:
    // 땅에 서 있는 적 진영(Chaser·Shooter·보스)이 가려는 쪽 발 앞이 비어 있으면 iMoveDir = 0
    void Apply(World& tWorld, const Tilemap& tRoom) const;

    static constexpr float kLookAhead = 16.0f;  // 발 앞 몇 px를 볼지 (Motor 감속 중 미끄러지는 거리 약 10px보다 크게)
};
