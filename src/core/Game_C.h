#pragma once

#include "audio/Audio_C.h"
#include "core/BenchScript_C.h"
#include "core/DebugIntent_C.h"
#include "core/FixedTimestep_C.h"
#include "core/Handle_U.h"
#include "core/MeasureRecorder_C.h"
#include "core/MenuIntent_C.h"
#include "core/Profiler_C.h"
#include "core/Tilemap_C.h"
#include "core/World_U.h"
#include "render/ViewCamera_C.h"
#include "render/Particles_C.h"
#include "render/Render_C.h"
#include "systems/Combat_C.h"
#include "systems/EnemyAI_U.h"
#include "systems/Input_C.h"
#include "systems/Items_C.h"
#include "systems/LedgeGuard_C.h"
#include "systems/Physics_C.h"
#include "systems/PlayerMotor_C.h"
#include "systems/Projectile_C.h"
#include "systems/RoomFlow_C.h"
#include "systems/SpatialQuery_C.h"
#include "systems/ShooterAI_C.h"
#include "systems/StressSpawner_C.h"

#include <optional>
#include <string>
#include <vector>

// 실행 옵션 (main 명령줄)
struct GameOptions
{
    bool bBenchMode = false;                    // --bench: 자동 측정 시나리오 후 종료 (measure.md 6장)
    SpatialMode eSpatialMode = SpatialMode::Hash;  // --full-scan: 전체 순회로 (비교 측정용)
    float fCellSize = SpatialHash::kDefaultCellSize;  // --cell=N: 해시 칸 크기 실험 (px)
};

// 시스템과 게임 데이터를 소유하고, 호출 순서만 정함. 게임 규칙 로직은 여기 넣지 않음.
class Game
{
public:
    explicit Game(const GameOptions& tOptions = {});

    void Run();  // 창이 닫힐 때까지 메인 루프

private:
    void FixedUpdate(float fDt);
    void UpdateCamera(float fFrameDt);  // 연출 (렌더 프레임마다)
    void UpdatePlayerStats();           // 아이템 modifier -> Stats::Calculate -> 최종 스탯
    void HandleKillDrops();             // 적 처치 -> 회복약 확률 드랍, 클리어 보상 자리 기억 (판정 결과를 보고)
    void QueueTickSounds(int iVolleys);  // 틱 결과(판정·발사·플레이어 동작 시작)를 보고 효과음 요청 (연출)
    // 측정·디버그 (GameDebug_C.cpp, measure.md)
    void HandleDebugKeys();                  // F1 디버그(무적), F5 측정 시작
    void UpdateBench(float fFrameSec);       // --bench: 시나리오 명령 실행 (새 게임·소환·측정 시작·종료)
    void SpawnStressEnemies(int iCount);     // 클리어 안 된 방에만, 풀이 꽉 차면 멈춤
    void FinishFrameMeasure(int iTicks);     // 프레임 끝: 측정 구간에 이번 프레임을 쌓고, 끝나면 CSV
    DebugInfo MakeDebugInfo(int iTicks) const;
    std::string GetSpatialLabel() const;     // 기록용 "full-scan" / "hash-128" (칸 크기 포함)
    std::string GetBenchCsvPath() const;     // bench_results_<검색 방식>.csv

    FixedTimestep m_tTimestep;
    Input m_tInput;
    EnemyAI m_tEnemyAI;  // 사용자 작성 (적 의도)
    ShooterAI m_tShooterAI;
    BossAI m_tBossAI;
    LedgeGuard m_tLedgeGuard;  // 적이 발판 끝에서 안 떨어지게 (이동 의도 거르기)
    Projectiles m_tProjectiles;
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
    DebugIntent m_tDebugIntent;         // F1·F2
    StressSpawner m_tStressSpawner;     // F2: 클리어 안 된 방에 작은 Chaser 추가 (측정용)

    Particles m_tParticles;                  // 연출 전용 (판정 무관)
    std::vector<HitReport> m_vecHitReports;  // 틱마다 Combat이 채움 -> 파티클 생성
    std::vector<Vec2> m_vecWallHits;         // 틱마다 투사체가 벽에 닿은 자리 -> 파편
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

    // 효과음 (연출). main의 AudioDeviceScope가 Game보다 먼저 생기고 늦게 사라짐 -> Sound 해제가 장치 닫기보다 먼저
    Audio m_tAudio;
    bool m_bPlayerWasDashing = false;              // 대시 "시작" 순간만 소리
    AttackPhase m_ePlayerAttackPhase = AttackPhase::Ready;  // 휘두름 "시작" 순간만 소리

    // 측정 (measure.md)
    FrameProfiler m_tProfiler;                  // 구간별 시간 (F1 패널)
    MeasureRecorder m_tRecorder;                // F5·벤치: 몇 초 평균 -> CSV
    SpatialQueryStats m_tFrameSpatial;          // 이번 프레임 틱들의 검색 비용 합
    std::optional<BenchScript> m_optBench;      // --bench일 때만 (C#의 nullable처럼 "없음"을 표현)
    bool m_bQuit = false;                       // 벤치 끝 -> 루프 종료

    int m_iHitStopTicks = 0;    // > 0 이면 고정 틱을 건너뜀 (히트스톱, 플레이어 피격 때)

    bool m_bShowDebug = false;  // F1
    long long m_llTotalTicks = 0;
};
