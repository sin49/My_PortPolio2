#pragma once
// 엔티티 종류 -> 진영 판정 (shooter.md 결정 9). "적인가?"를 묻는 곳은 전부 여기를 거침
// (Chaser AI처럼 "이 종류만" 고르는 곳은 EntityType을 직접 비교)
#include "core/Entity_U.h"

// 적 진영 몸 (Chaser·Shooter·보스). 방 클리어·남은 적 표시·미니맵·피격 대상
inline bool IsEnemySide(EntityType eType)
{
    return eType == EntityType::Enemy || eType == EntityType::Shooter || eType == EntityType::Boss;
}

// 맞을 수 있는 몸 (드랍·투사체 제외)
inline bool IsHittable(EntityType eType)
{
    return eType == EntityType::Player || IsEnemySide(eType);
}

// 몸을 가진 종류의 진영. 투사체는 자기 tProjectile.eOwner를 씀
inline Faction GetFaction(EntityType eType)
{
    return eType == EntityType::Player ? Faction::Player : Faction::Enemy;
}
