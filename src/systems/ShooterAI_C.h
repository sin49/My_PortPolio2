#pragma once
// Shooter FSM (shooter.md). 사용자 EnemyAI(Chaser)와 같은 모양: enum 상태 + Enter + 타이머 전이, 의도만 씀.
// 상태 저장은 사용자 EnemyAIState 그대로 재사용 (결정 3-A): Chase = "거리 맞추기", Attack = 1발 발사
#include "core/Handle_U.h"

class World;

class ShooterAI
{
public:
    void Update(World& tWorld, Handle hPlayer, float fDt) const;
};
