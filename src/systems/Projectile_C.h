#pragma once
// 투사체 (docs/design/boss-projectile.md). Entity가 이 헤더를 include함 -> 여기선 Entity/World를 include하지 않음
#include "core/Handle_U.h"
#include "core/Vec2_C.h"

#include <vector>

class Tilemap;
class World;
struct ProjectileDef;

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

// 투사체 풀 = 사용자 World 그대로 (EntityType::Projectile). 따로 풀을 만들지 않음 (boss-projectile.md 결정 1-A)
class Projectiles
{
public:
    // vCenter에서 vDir 방향(길이 무관)으로. World가 꽉 차면 World가 throw (복구 안 하는 치명적 오류)
    Handle Spawn(World& tWorld, Vec2 vCenter, Vec2 vDir, const ProjectileDef& tDef, Faction eOwner) const;

    // 발사 의도(AttackIntent.bFirePressed)를 모아서 생성. 순회 중엔 모으기만, 생성은 순회가 끝난 뒤
    // 반환: 이번에 쏜 발사 횟수 (탄 수 아님. 효과음용)
    int FireFromIntents(World& tWorld, const ProjectileDef& tDef);

    // 직선 이동 (중력·Motor X) + 수명·벽 검사. 벽에 닿은 위치는 vecOutWallHits에 (파편용, 호출자가 비움)
    void Update(World& tWorld, const Tilemap& tMap, float fDt, std::vector<Vec2>& vecOutWallHits) const;

private:
    struct FireRequest
    {
        Vec2 vCenter = {};
        Vec2 vAim = {};
        int iShotCount = 1;
        float fSpreadDeg = 0.0f;
        Faction eOwner = Faction::Enemy;
    };
    std::vector<FireRequest> m_vecRequests;  // 재사용 버퍼
};
