#pragma once

#include "core/FixedTimestep_C.h"
#include "core/Handle_U.h"
#include "core/Tilemap_C.h"
#include "core/World_U.h"
#include "render/Particles_C.h"
#include "render/Render_C.h"
#include "systems/Combat_C.h"
#include "systems/Input_C.h"
#include "systems/Physics_C.h"
#include "systems/PlayerMotor_C.h"
#include "systems/SpatialQuery_C.h"

#include <vector>

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

    World m_tWorld;
    SpatialQuery m_tSpatial{m_tWorld};  // m_tWorld를 빌려 씀 -> 반드시 m_tWorld 아래에 선언 (멤버는 선언 순서대로 생성)
    Handle m_hPlayer;  // 플레이어는 핸들로만 보관. 쓸 때마다 m_tWorld.Get()으로 꺼냄

    Particles m_tParticles;                   // 연출 전용 (판정 무관)
    std::vector<HitReport> m_vecHitReports;   // 틱마다 Combat이 채움 -> 파티클 생성

    long long m_llTotalTicks = 0;
};
