// Game의 연출 부분: 카메라(따라가기·흔들림), 효과음 요청. 판정과 무관 (Game_C.cpp가 300줄을 넘지 않게 분리, 같은 클래스)
#include "core/Game_C.h"

namespace
{
Vec2 GetRoomSizePx(const Tilemap& tRoom)
{
    return Vec2{static_cast<float>(tRoom.GetCols()) * kTileSize, static_cast<float>(tRoom.GetRows()) * kTileSize};
}
} // namespace

void Game::UpdateCamera(float fFrameDt)
{
    m_tCamera.UpdateShake(fFrameDt);  // 히트스톱으로 틱이 멈춰도 렌더 프레임마다 흔들림은 진행
    const Entity* pPlayer = m_tWorld.Get(m_hPlayer);
    if (pPlayer == nullptr)
        return;
    const Body& tBody = pPlayer->tBody;
    const Vec2 vFocus = {tBody.vPos.fX + tBody.vSize.fX * 0.5f, tBody.vPos.fY + tBody.vSize.fY * 0.5f};
    const Vec2 vRoom = GetRoomSizePx(m_tRoom);

    const bool bRoomChanged = m_iCameraRoomIndex != m_tRoomFlow.GetRoomIndex() || !(m_hCameraPlayer == m_hPlayer);
    if (bRoomChanged)
    {
        m_tCamera.SnapTo(vFocus, kViewSize, vRoom);
        m_iCameraRoomIndex = m_tRoomFlow.GetRoomIndex();
        m_hCameraPlayer = m_hPlayer;
        return;
    }
    m_tCamera.Update(vFocus, kViewSize, vRoom, fFrameDt);
}

void Game::QueueTickSounds(int iVolleys)
{
    for (const HitReport& tReport : m_vecHitReports)
        m_tAudio.Queue(tReport.bTargetIsPlayer ? SfxId::Hurt : tReport.bKilled ? SfxId::Kill : SfxId::Hit);
    if (iVolleys > 0)
        m_tAudio.Queue(SfxId::Shoot);
    if (!m_vecPickups.empty())
        m_tAudio.Queue(SfxId::Pickup);

    // 플레이어 동작은 "시작한 순간"만 (지난 틱 상태와 비교)
    const Entity* pPlayer = m_tWorld.Get(m_hPlayer);
    if (pPlayer == nullptr)
        return;
    const bool bDashing = pPlayer->tMotor.tDash.IsDashing();
    if (bDashing && !m_bPlayerWasDashing)
        m_tAudio.Queue(SfxId::Dash);
    m_bPlayerWasDashing = bDashing;

    const AttackPhase ePhase = pPlayer->tAttack.ePhase;
    if (ePhase == AttackPhase::Active && m_ePlayerAttackPhase != AttackPhase::Active)
        m_tAudio.Queue(SfxId::Swing);
    m_ePlayerAttackPhase = ePhase;
}
