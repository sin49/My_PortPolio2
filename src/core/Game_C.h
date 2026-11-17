#pragma once

#include "core/FixedTimestep_C.h"
#include "core/Handle_U.h"
#include "core/MenuIntent_C.h"
#include "core/Tilemap_C.h"
#include "core/World_U.h"
#include "render/ViewCamera_C.h"
#include "render/Particles_C.h"
#include "render/Render_C.h"
#include "systems/Combat_C.h"
#include "systems/Input_C.h"
#include "systems/Physics_C.h"
#include "systems/PlayerMotor_C.h"
#include "systems/RoomFlow_C.h"
#include "systems/SpatialQuery_C.h"

#include <vector>

// 시스템과 게임 데이터를 소유하고, 호출 순서만 정함. 게임 규칙 로직은 여기 넣지 않음.
class Game
{
public:
    Game();

    void Run();  // 창이 닫힐 때까지 메인 루프

private:
    void FixedUpdate(float fDt);
    void UpdateCamera(float fFrameDt);  // 연출 (렌더 프레임마다)

    FixedTimestep m_tTimestep;
    Input m_tInput;
    PlayerMotor m_tPlayerMotor;
    Physics m_tPhysics;
    Combat m_tCombat;
    Render m_tRender;

    // 멤버는 선언 순서대로 생성 -> 빌려 쓰는 쪽은 빌려 주는 쪽 아래에
    Tilemap m_tRoom;
    RoomFlow m_tRoomFlow{m_tRoom};      // m_tRoom을 빌려 씀 (방 전환 때 타일 교체)
    World m_tWorld;
    SpatialQuery m_tSpatial{m_tWorld};  // m_tWorld를 빌려 씀
    Handle m_hPlayer;                   // 플레이어는 핸들로만 보관. 쓸 때마다 m_tWorld.Get()으로 꺼냄
    MenuIntent m_tMenuIntent;

    Particles m_tParticles;                  // 연출 전용 (판정 무관)
    std::vector<HitReport> m_vecHitReports;  // 틱마다 Combat이 채움 -> 파티클 생성
    ViewCamera m_tCamera;                        // 연출 전용
    int m_iCameraRoomIndex = -1;             // 방이 바뀌면 카메라를 바로 옮기려고
    Handle m_hCameraPlayer;                  // 새 게임(새 플레이어 핸들)이면 바로 옮기려고

    bool m_bShowDebug = false;  // F1
    long long m_llTotalTicks = 0;
};
