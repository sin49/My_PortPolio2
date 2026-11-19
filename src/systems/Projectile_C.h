#pragma once
// 투사체 상태 (docs/design/boss-projectile.md). Entity가 이 헤더를 include함 -> 여기선 Entity/World를 include하지 않음

// 누가 쐈나. EntityType 대신 따로 둔 이유: EntityType은 Entity_U.h에 있어서 include하면 순환이 생김
enum class Faction
{
    Player,
    Enemy,
};

struct ProjectileState
{
    float fLife = 0.0f;    // 남은 수명(초). 0 이하 = 삭제 예약됨
    float fDamage = 0.0f;
    Faction eOwner = Faction::Enemy;
};
