#include "systems/Combat_C.h"

#include "data/PlayerDef_C.h"

#include <algorithm>

namespace
{
// 몸 앞쪽(iFacing 방향)에 붙은 사각형, 세로는 몸 가운데 맞춤
Rect ComputeHitbox(const Body& tBody, int iFacing)
{
    const Vec2 vSize = kPlayerDef.vAttackSize;
    const float fX = iFacing > 0 ? tBody.vPos.fX + tBody.vSize.fX : tBody.vPos.fX - vSize.fX;
    const float fY = tBody.vPos.fY + (tBody.vSize.fY - vSize.fY) * 0.5f;
    return Rect{fX, fY, vSize.fX, vSize.fY};
}
} // namespace

void Combat::UpdatePlayerAttack(const AttackIntent& tIntent, const Body& tBody, int iFacing, AttackState& tAttack,
                                float fDt) const
{
    tAttack.fBufferTimer =
        tIntent.bAttackPressed ? kPlayerDef.fAttackBufferTime : std::max(0.0f, tAttack.fBufferTimer - fDt);

    // 단계 진행
    if (tAttack.ePhase == AttackPhase::Active)
    {
        tAttack.fTimer -= fDt;
        if (tAttack.fTimer <= 0.0f)
        {
            tAttack.ePhase = AttackPhase::Recover;
            tAttack.fTimer = kPlayerDef.fAttackRecoverTime;
        }
    }
    else if (tAttack.ePhase == AttackPhase::Recover)
    {
        tAttack.fTimer -= fDt;
        if (tAttack.fTimer <= 0.0f)
            tAttack.ePhase = AttackPhase::Ready;
    }

    // 시작: Recover가 끝난 같은 틱에도 버퍼가 남아 있으면 바로 시작
    if (tAttack.ePhase == AttackPhase::Ready && tAttack.fBufferTimer > 0.0f)
    {
        tAttack.ePhase = AttackPhase::Active;
        tAttack.fTimer = kPlayerDef.fAttackActiveTime;
        tAttack.fBufferTimer = 0.0f;
        // TODO(World): vecAlreadyHit.clear()
    }

    if (tAttack.ePhase == AttackPhase::Active)
    {
        tAttack.rcHitbox = ComputeHitbox(tBody, iFacing);  // 매 틱 다시 계산 -> 몸을 따라감
        // TODO(World): SpatialQuery::QueryNearby(rcHitbox) -> 겹치고 vecAlreadyHit에 없으면 DamageEvent
    }
}
