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
#include "systems/EnemyAI_U.h"
#include "systems/Input_C.h"
#include "systems/Items_C.h"
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
    void UpdatePlayerStats();           // 아이템 modifier -> Stats::Calculate -> 최종 스탯
    void HandleKillDrops();             // 적 처치 -> 회복약 확률 드랍, 클리어 보상 자리 기억 (판정 결과를 보고)

    FixedTimestep m_tTimestep;
    Input m_tInput;
    EnemyAI m_tEnemyAI;  // 사용자 작성 (적 의도)
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

    // 아이템·스탯 (items.md). 기본값 kPlayerBaseStats는 안 바뀌고 m_tPlayerStats는 매 틱 다시 계산한 결과
    Items m_tItems;
    StatBlock m_tPlayerStats = kPlayerBaseStats;
    std::vector<StatModifier> m_vecStatMods;     // 매 틱 재사용 버퍼
    std::vector<PickupReport> m_vecPickups;      // 매 틱 재사용 버퍼
    Handle m_hStatsPlayer;                       // 새 게임(새 플레이어 핸들)이면 아이템 초기화
    Vec2 m_vLastKillPos = {};                    // 클리어 보상이 떨어질 자리
    PickupNotice m_tPickupNotice;                // 화면 알림 (연출)

    bool m_bShowDebug = false;  // F1
    long long m_llTotalTicks = 0;
};
