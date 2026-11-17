#pragma once

#include "core/Handle_U.h"
#include "core/MenuIntent_C.h"
#include "core/Tilemap_C.h"

class World;
struct RoomDef;

enum class GamePhase
{
    Title,
    Playing,
    GameOver,
    Victory,
};

enum class RoomState
{
    Locked,   // 적 남음, 문 닫힘
    Cleared,  // 전멸, 문 열림
};

// 방 진행·승패 판단만 (GDD 7장). 그리기 X. 방 타일과 엔티티 배치는 바꿈 (방 전환 담당).
class RoomFlow
{
public:
    // 타이틀부터. tRoom은 첫 방(튜토리얼) 모양으로 채워 둠 (타이틀 뒤 배경)
    explicit RoomFlow(Tilemap& tRoom);

    // 매 고정 틱 맨 앞. hPlayer: 새 게임 시작 때 새 핸들로 바뀜
    void Update(World& tWorld, Handle& hPlayer, const MenuIntent& tMenu, float fDt);

    GamePhase GetPhase() const { return m_ePhase; }
    RoomState GetRoomState() const { return m_eRoomState; }
    int GetRoomIndex() const { return m_iRoomIndex; }
    const RoomDef& GetRoomDef() const;
    int GetRoomEnemyTotal() const;  // 방 시작 때 적 수 (남은 적 방향 표시 기준)
    float GetPlayTime() const { return m_fPlayTime; }  // 승리 화면용

    // 바로 새 게임 시작 (Playing). 테스트에서도 사용
    void StartNewGame(World& tWorld, Handle& hPlayer);
    // 타이틀 화면 뒤 배경: 첫 방을 깔아 두되 단계는 Title (Enter를 누르면 새로 시작)
    void PrepareTitle(World& tWorld, Handle& hPlayer);

private:
    void LoadRoom(World& tWorld, Handle hPlayer, int iRoomIndex);

    Tilemap& m_tRoom;  // Game 소유, 빌려 씀 (수명 같음)
    GamePhase m_ePhase = GamePhase::Title;
    RoomState m_eRoomState = RoomState::Locked;
    int m_iRoomIndex = 0;
    float m_fPlayTime = 0.0f;
};
