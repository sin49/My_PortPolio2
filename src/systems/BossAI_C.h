#pragma once
// 보스 FSM (boss-projectile.md 4장, shooter.md 결정 6-A). 사용자 EnemyAI와 같은 모양: enum 상태 + Enter + 타이머 전이.
// 의도만 씀: 이동 = MoveIntent, 발사 = AttackIntent.bFirePressed (탄 생성은 Projectiles가 순회 밖에서)
// Entity가 이 헤더를 include함 -> Entity를 아는 헤더는 include 금지 (순환). Handle만
#include "core/Handle_U.h"

class World;

enum class BossState
{
    Idle,
    AimedWindup,  // 조준 3연사 예고
    AimedFire,
    RingWindup,   // 8방향 원형탄 예고
    RingFire,
    Recover,      // 플레이어 쪽으로 천천히 걸음
};

struct BossAIState
{
    BossState eState = BossState::Idle;
    float fTimer = 0.0f;
    bool bNextIsRing = false;  // Recover 다음 패턴 (조준 <-> 원형 번갈아)
};

class BossAI
{
public:
    void Update(World& tWorld, Handle hPlayer, float fDt) const;
};
