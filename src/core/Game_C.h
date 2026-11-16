#pragma once

#include "core/AttackIntent_C.h"
#include "core/Body_C.h"
#include "core/FixedTimestep_C.h"
#include "core/MoveIntent_C.h"
#include "core/Tilemap_C.h"
#include "data/PlayerDef_C.h"
#include "data/TestRoom_C.h"
#include "render/Render_C.h"
#include "systems/Combat_C.h"
#include "systems/Input_C.h"
#include "systems/Physics_C.h"
#include "systems/PlayerMotor_C.h"

// 시스템과 게임 데이터를 소유하고, 호출 순서만 정함. 게임 규칙 로직은 여기 넣지 않음.
class Game
{
public:
    explicit Game(Tilemap tStartRoom);

    void Run();  // 창이 닫힐 때까지 메인 루프

private:
    void FixedUpdate(float fDt);

    FixedTimestep m_tTimestep;
    Input m_tInput;
    PlayerMotor m_tPlayerMotor;
    Physics m_tPhysics;
    Combat m_tCombat;
    Render m_tRender;

    Tilemap m_tRoom;

    // TODO(World): 핸들 풀이 생기면 플레이어를 World로 옮김.
    // 시스템들은 Body& / 상태& 만 받으므로 옮겨도 호출부는 그대로.
    MoveIntent m_tPlayerMoveIntent;
    AttackIntent m_tPlayerAttackIntent;
    Body m_tPlayerBody = {.vPos = kTestRoomPlayerSpawn, .vSize = kPlayerDef.vSize};
    PlayerMotorState m_tPlayerMotorState;
    AttackState m_tPlayerAttack;

    long long m_llTotalTicks = 0;
};
