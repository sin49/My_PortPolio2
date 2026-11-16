#include "core/AttackIntent_C.h"
#include "core/Body_C.h"
#include "core/FixedTimestep_C.h"
#include "data/PlayerDef_C.h"
#include "systems/Combat_C.h"

#include <doctest/doctest.h>

namespace
{
constexpr float kDt = FixedTimestep::kStep;
constexpr AttackIntent kNone = {};
constexpr AttackIntent kPress = {.bAttackPressed = true};

constexpr Body kBody = {.vPos = {100.0f, 200.0f}, .vSize = {24.0f, 40.0f}};

struct Sim
{
    Combat tCombat;
    AttackState tAttack;
    int iFacing = 1;

    void Tick(const AttackIntent& tIntent) { tCombat.UpdatePlayerAttack(tIntent, kBody, iFacing, tAttack, kDt); }

    // 지정한 단계가 끝날 때까지 진행, 걸린 틱 수 반환
    int RunWhile(AttackPhase ePhase)
    {
        int iTicks = 0;
        while (tAttack.ePhase == ePhase && iTicks < 120)
        {
            Tick(kNone);
            ++iTicks;
        }
        return iTicks;
    }
};
} // namespace

TEST_CASE("Combat: 누르면 Active -> Recover -> Ready")
{
    Sim tSim;
    CHECK(tSim.tAttack.ePhase == AttackPhase::Ready);

    tSim.Tick(kPress);
    CHECK(tSim.tAttack.ePhase == AttackPhase::Active);

    const int iActiveTicks = tSim.RunWhile(AttackPhase::Active);  // 0.15초 ≈ 9틱
    CHECK(iActiveTicks >= 8);
    CHECK(iActiveTicks <= 10);
    CHECK(tSim.tAttack.ePhase == AttackPhase::Recover);

    const int iRecoverTicks = tSim.RunWhile(AttackPhase::Recover);  // 0.20초 ≈ 12틱
    CHECK(iRecoverTicks >= 11);
    CHECK(iRecoverTicks <= 13);
    CHECK(tSim.tAttack.ePhase == AttackPhase::Ready);
}

TEST_CASE("Combat: 히트박스는 바라보는 쪽에 붙음")
{
    Sim tRight;
    tRight.Tick(kPress);
    CHECK(tRight.tAttack.rcHitbox.fX == doctest::Approx(kBody.vPos.fX + kBody.vSize.fX));
    CHECK(tRight.tAttack.rcHitbox.fW == doctest::Approx(kPlayerDef.vAttackSize.fX));

    Sim tLeft;
    tLeft.iFacing = -1;
    tLeft.Tick(kPress);
    CHECK(tLeft.tAttack.rcHitbox.fX + tLeft.tAttack.rcHitbox.fW == doctest::Approx(kBody.vPos.fX));

    // 세로는 몸 가운데
    const float fHitboxCenterY = tRight.tAttack.rcHitbox.fY + tRight.tAttack.rcHitbox.fH * 0.5f;
    CHECK(fHitboxCenterY == doctest::Approx(kBody.vPos.fY + kBody.vSize.fY * 0.5f));
}

TEST_CASE("Combat: 버퍼 - Recover 끝나기 직전 입력은 바로 다음 공격으로")
{
    Sim tSim;
    tSim.Tick(kPress);
    tSim.RunWhile(AttackPhase::Active);
    REQUIRE(tSim.tAttack.ePhase == AttackPhase::Recover);

    for (int i = 0; i < 8; ++i)  // Recover 약 12틱 중 8틱 진행 -> 남은 시간 < 버퍼 0.12초
        tSim.Tick(kNone);
    tSim.Tick(kPress);
    REQUIRE(tSim.tAttack.ePhase == AttackPhase::Recover);  // 아직은 대기 중

    bool bRestarted = false;
    for (int i = 0; i < 6 && !bRestarted; ++i)
    {
        tSim.Tick(kNone);
        bRestarted = tSim.tAttack.ePhase == AttackPhase::Active;
    }
    CHECK(bRestarted);
}

TEST_CASE("Combat: 버퍼 - 너무 일찍 누른 입력은 버려짐")
{
    Sim tSim;
    tSim.Tick(kPress);
    tSim.Tick(kNone);
    tSim.Tick(kPress);  // Active 중 입력 -> Ready까지 0.3초 넘게 남음 > 버퍼 0.12초

    tSim.RunWhile(AttackPhase::Active);
    tSim.RunWhile(AttackPhase::Recover);
    CHECK(tSim.tAttack.ePhase == AttackPhase::Ready);

    for (int i = 0; i < 10; ++i)
        tSim.Tick(kNone);
    CHECK(tSim.tAttack.ePhase == AttackPhase::Ready);
}
