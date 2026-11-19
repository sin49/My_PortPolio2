#pragma once

#include "core/AttackIntent_C.h"
#include "core/Body_C.h"
#include "core/Handle_U.h"
#include "core/Rect_C.h"
#include "core/Vec2_C.h"
#include "data/AttackDef_C.h"
#include "data/PlayerDef_C.h"

#include <array>
#include <vector>

// Entity_U.h가 이 헤더를 include하므로 World는 전방 선언만 (순환 include 방지)
struct Entity;
class SpatialQuery;
class World;

enum class AttackPhase
{
    Ready,    // 공격 가능
    Active,   // 히트박스 유효
    Recover,  // 다음 공격까지 대기
};

// 공격 히트박스 위치 계산 (순수 함수). Combat이 Active 때, Render가 적 예고(Windup) 범위를 그릴 때 씀
Rect ComputeAttackHitbox(const Body& tBody, int iFacing, Vec2 vSize);

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
    float fInvulnTimer = 0.0f; // > 0 이면 안 맞음 (플레이어 피격 무적, 11/19). 대시 무적과 별개
};

// 판정 결과 1건. 파티클 등 연출 쪽이 읽음 (Combat은 연출을 모름)
struct HitReport
{
    Vec2 vPos = {};  // 히트박스와 몸이 겹친 부분의 가운데
    int iDir = 1;    // 공격자 -> 맞은 쪽 방향 (-1 / +1)
    bool bKilled = false;  // 이 타격으로 HP 0이 됨
    float fScale = 1.0f;   // 맞은 대상 크기 (작은 적 키 = 1.0). 연출이 파편 규모에 씀
    bool bTargetIsPlayer = false;  // 맞은 쪽이 플레이어 (적 처치 보상은 false일 때만)
};

class Combat
{
public:
    // 공격 단계·히트박스 갱신 (플레이어·적 공용, 종류별 차이는 tDef). iFacing: PlayerMotorState::iFacing (-1 / +1).
    // tBody는 Physics 이동이 끝난 위치.
    void UpdateAttack(const AttackIntent& tIntent, const Body& tBody, int iFacing, const AttackDef& tDef,
                      AttackState& tAttack, float fDt) const;

    // 모든 공격 단계 갱신 뒤 호출. Active 히트박스 겹침 -> HP 차감·넉백·깜빡임, HP 0인 적은 Destroy 예약.
    // 맞은 것마다 vecOutReports에 추가 (호출자가 비우고 넘김). fPlayerDamage: 플레이어 최종 공격력 (Stats 결과)
    void ResolveHits(World& tWorld, const SpatialQuery& tSpatial, std::vector<HitReport>& vecOutReports, float fDt,
                     float fPlayerDamage = kPlayerDef.fAttackDamage);

    // 투사체 몸이 진영이 다른 몸과 겹치면: HP 차감·넉백·깜빡임 + 탄 삭제 (관통 X, 1명만). ResolveHits 다음에 호출
    void ResolveProjectileHits(World& tWorld, const SpatialQuery& tSpatial, std::vector<HitReport>& vecOutReports);

    // 디버그 무적 (F1 디버그 모드, 스트레스 테스트용): 켜면 플레이어는 근접·탄 모두 안 맞음 (탄은 지나감)
    void SetPlayerInvincible(bool bOn) { m_bPlayerInvincible = bOn; }
    bool IsPlayerInvincible() const { return m_bPlayerInvincible; }

private:
    bool CanBeHit(const Entity& tTarget) const;  // 죽는 중·대시 무적·디버그 무적이면 false

    bool m_bPlayerInvincible = false;
    std::vector<Handle> m_vecQueryBuffer;  // QueryNearby 결과 재사용 (첫 몇 틱 뒤로는 할당 없음)
};
