#pragma once

#include "core/AttackIntent_C.h"
#include "core/Body_C.h"
#include "core/Handle_U.h"
#include "core/Rect_C.h"
#include "core/Vec2_C.h"
#include "data/AttackDef_C.h"

#include <array>
#include <vector>

// Entity_U.h가 이 헤더를 include하므로 World는 전방 선언만 (순환 include 방지)
class SpatialQuery;
class World;

enum class AttackPhase
{
    Ready,    // 공격 가능
    Active,   // 히트박스 유효
    Recover,  // 다음 공격까지 대기
};

inline constexpr int kMaxHitsPerSwing = 16;  // 한 번 휘두를 때 맞힐 수 있는 최대 수 (넘으면 무시)

// 근접 공격 상태. 히트박스는 엔티티가 아니라 이 안의 값 -> 생성/삭제 없음.
struct AttackState
{
    AttackPhase ePhase = AttackPhase::Ready;
    float fTimer = 0.0f;        // 현재 단계 남은 시간
    float fBufferTimer = 0.0f;  // > 0 이면 최근에 공격을 눌렀음
    Rect rcHitbox = {};         // Active일 때만 의미 있음

    // 이번 휘두름에 이미 맞은 대상 (포인터 X, 핸들 O). 공격 시작 때 비움
    std::array<Handle, kMaxHitsPerSwing> arrAlreadyHit = {};
    int iAlreadyHitCount = 0;
};

// 맞은 쪽 상태. 나중에 경직·무적 타이머도 여기에 추가 (Entity 수정 없이)
struct HitState
{
    float fFlashTimer = 0.0f;  // > 0 이면 흰색 깜빡임
};

// 판정 결과 1건. 파티클 등 연출 쪽이 읽음 (Combat은 연출을 모름)
struct HitReport
{
    Vec2 vPos = {};  // 히트박스와 몸이 겹친 부분의 가운데
    int iDir = 1;    // 공격자 -> 맞은 쪽 방향 (-1 / +1)
    bool bKilled = false;  // 이 타격으로 HP 0이 됨
    float fScale = 1.0f;   // 맞은 대상 크기 (작은 적 키 = 1.0). 연출이 파편 규모에 씀
};

class Combat
{
public:
    // 공격 단계·히트박스 갱신 (플레이어·적 공용, 종류별 차이는 tDef). iFacing: PlayerMotorState::iFacing (-1 / +1).
    // tBody는 Physics 이동이 끝난 위치.
    void UpdateAttack(const AttackIntent& tIntent, const Body& tBody, int iFacing, const AttackDef& tDef,
                      AttackState& tAttack, float fDt) const;

    // 모든 공격 단계 갱신 뒤 호출. Active 히트박스 겹침 -> HP 차감·넉백·깜빡임, HP 0인 적은 Destroy 예약.
    // 맞은 것마다 vecOutReports에 추가 (호출자가 비우고 넘김)
    void ResolveHits(World& tWorld, const SpatialQuery& tSpatial, std::vector<HitReport>& vecOutReports, float fDt);

private:
    std::vector<Handle> m_vecQueryBuffer;  // QueryNearby 결과 재사용 (첫 몇 틱 뒤로는 할당 없음)
};
