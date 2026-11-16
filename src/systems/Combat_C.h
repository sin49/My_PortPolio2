#pragma once

#include "core/AttackIntent_C.h"
#include "core/Body_C.h"
#include "core/Rect_C.h"

enum class AttackPhase
{
    Ready,    // 공격 가능
    Active,   // 히트박스 유효
    Recover,  // 다음 공격까지 대기
};

// 근접 공격 상태. 히트박스는 엔티티가 아니라 이 안의 값 -> 생성/삭제 없음.
struct AttackState
{
    AttackPhase ePhase = AttackPhase::Ready;
    float fTimer = 0.0f;        // 현재 단계 남은 시간
    float fBufferTimer = 0.0f;  // > 0 이면 최근에 공격을 눌렀음
    Rect rcHitbox = {};         // Active일 때만 의미 있음
    // TODO(World): std::vector<Handle> vecAlreadyHit -> 한 번 휘두를 때 같은 적 1회만 (포인터 X, 핸들 O)
};

// 히트박스 겹침 -> 데미지 (적 추가 후). 지금은 공격 단계·히트박스만 관리.
class Combat
{
public:
    // iFacing: PlayerMotorState::iFacing (-1 / +1). tBody는 Physics 이동이 끝난 위치.
    void UpdatePlayerAttack(const AttackIntent& tIntent, const Body& tBody, int iFacing, AttackState& tAttack,
                            float fDt) const;
};
