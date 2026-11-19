#pragma once

#include "core/Entity_U.h"
#include "data/EnemyDef_C.h"
#include "data/PlayerDef_C.h"
#include "data/Rooms_C.h"

// 종류별 초기 Entity 값 (생성 규칙을 한 곳에). World.Create에 넘김.
inline Entity MakePlayerEntity(Vec2 vPos)
{
    Entity tPlayer;
    tPlayer.eType = EntityType::Player;
    tPlayer.tBody = {.vPos = vPos, .vSize = kPlayerDef.vSize};
    tPlayer.fHp = kPlayerDef.fMaxHp;
    return tPlayer;
}

// vFeet: 발 위치 (가운데 아래). 방 데이터는 바닥 기준이라 이게 편함
inline Entity MakeEnemyEntity(const EnemySpawn& tSpawn, Vec2 vFeet)
{
    // TODO: Shooter·Boss 타입이 생기면 종류별 Def로. 지금은 전부 Chaser (크기만 다름)
    // 튜토리얼 허수아비: AI가 건드리지 않음 (EnemyAI의 bPassive 검사)
    const EnemySizeDef& tSizeDef = GetEnemySizeDef(tSpawn.eSize);
    const Vec2 vSize = tSizeDef.vSize;
    Entity tEnemy;
    tEnemy.eType = EntityType::Enemy;
    tEnemy.tBody = {.vPos = {vFeet.fX - vSize.fX * 0.5f, vFeet.fY - vSize.fY}, .vSize = vSize};
    tEnemy.fHp = tSizeDef.fMaxHp;
    tEnemy.tMotor.iFacing = -1;  // 방은 왼쪽에서 시작 -> 적은 왼쪽(플레이어 쪽)을 봄
    tEnemy.tAI.bPassive = tSpawn.eKind == SpawnKind::Dummy;
    return tEnemy;
}
