#include "render/Render_C.h"

#include "core/Body_C.h"
#include "core/Rect_C.h"
#include "core/Tilemap_C.h"
#include "core/World_U.h"
#include "data/EnemyDef_C.h"
#include "data/PlayerDef_C.h"
#include "data/Rooms_C.h"
#include "render/ViewCamera_C.h"
#include "render/Hud_C.h"
#include "render/Particles_C.h"
#include "systems/Combat_C.h"
#include "systems/PlayerMotor_C.h"
#include "systems/RoomFlow_C.h"

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace
{
constexpr int kTilePx = static_cast<int>(kTileSize);

constexpr Color kFrameColor = {0, 0, 0, 255};
constexpr Color kRoomBackground = {24, 20, 37, 255};
constexpr Color kSolidColor = {88, 80, 110, 255};
constexpr Color kSolidTopColor = {120, 110, 150, 255};
constexpr Color kPlayerColor = {232, 196, 92, 255};
constexpr Color kPlayerRunColor = {110, 210, 240, 255};
constexpr Color kPlayerDashColor = {200, 245, 255, 255};
constexpr Color kGaugeExhaustedColor = {90, 90, 110, 255};
constexpr Color kPlayerGroundedOutline = {255, 255, 255, 255};
constexpr Color kHitboxColor = {240, 80, 80, 140};
constexpr Color kEnemyColor = {200, 70, 90, 255};
constexpr Color kHitFlashColor = {255, 255, 255, 255};
constexpr Color kHpBarColor = {120, 220, 120, 255};
constexpr Color kHpBarBackColor = {40, 40, 40, 255};
constexpr Color kSparkColor = {255, 255, 255, 255};
constexpr Color kDoorLockedColor = {170, 50, 60, 255};
constexpr Color kDoorOpenColor = {90, 220, 140, 255};
constexpr Color kHintColor = {230, 230, 240, 200};
constexpr Color kDebugTextColor = {220, 220, 220, 255};

// raylib의 Rectangle은 외부 타입이라 필드명(x, y, width, height)이 raylib 것 그대로임.
// 카메라 모드(BeginMode2D) 안에서는 방 좌표 그대로 그리면 됨.
Rectangle ToRaylib(const Rect& rcArea)
{
    return Rectangle{rcArea.fX, rcArea.fY, rcArea.fW, rcArea.fH};
}

// 화면에 보이는 타일만 그림 (넓은 방 대비)
void DrawRoom(const Tilemap& tRoom, Vec2 vCameraTopLeft)
{
    DrawRectangle(0, 0, tRoom.GetCols() * kTilePx, tRoom.GetRows() * kTilePx, kRoomBackground);
    const int iMinX = std::max(0, static_cast<int>(vCameraTopLeft.fX) / kTilePx - 1);
    const int iMinY = std::max(0, static_cast<int>(vCameraTopLeft.fY) / kTilePx - 1);
    const int iMaxX = std::min(tRoom.GetCols() - 1, iMinX + kScreenWidth / kTilePx + 2);
    const int iMaxY = std::min(tRoom.GetRows() - 1, iMinY + kViewHeight / kTilePx + 2);
    for (int iTileY = iMinY; iTileY <= iMaxY; ++iTileY)
    {
        for (int iTileX = iMinX; iTileX <= iMaxX; ++iTileX)
        {
            if (!tRoom.IsSolid(iTileX, iTileY))
                continue;
            DrawRectangle(iTileX * kTilePx, iTileY * kTilePx, kTilePx, kTilePx, kSolidColor);
            if (!tRoom.IsSolid(iTileX, iTileY - 1))  // 윗면 강조 -> 발판이 잘 보이게
                DrawRectangle(iTileX * kTilePx, iTileY * kTilePx, kTilePx, 4, kSolidTopColor);
        }
    }
}

void DrawDoor(const RoomFlow& tRoomFlow)
{
    const RoomDef& tDef = tRoomFlow.GetRoomDef();
    if (tDef.bBossRoom)
        return;
    const bool bOpen = tRoomFlow.GetRoomState() == RoomState::Cleared;
    const Rectangle tDoor = ToRaylib(GetRoomDoor(tDef));
    DrawRectangleRec(tDoor, Fade(bOpen ? kDoorOpenColor : kDoorLockedColor, bOpen ? 0.55f : 0.9f));
    DrawRectangleLinesEx(tDoor, 2.0f, bOpen ? kDoorOpenColor : kDoorLockedColor);
}

// 튜토리얼 키캡: 글자는 키 이름만
void DrawHints(const RoomDef& tDef)
{
    constexpr int kFontSize = 20;
    for (const RoomHint& tHint : tDef.spHints)
    {
        const std::string strKeys(tHint.svKeys);
        const int iTextW = MeasureText(strKeys.c_str(), kFontSize);
        const float fX = tHint.fTileX * kTileSize;
        const float fY = tHint.fTileY * kTileSize;
        const Rectangle tCap = {fX - 8.0f, fY - 6.0f, static_cast<float>(iTextW) + 16.0f, 32.0f};
        DrawRectangleRoundedLinesEx(tCap, 0.3f, 4, 2.0f, kHintColor);
        DrawText(strKeys.c_str(), static_cast<int>(fX), static_cast<int>(fY), kFontSize, kHintColor);
        if (!tHint.svAction.empty())
        {
            const std::string strAction(tHint.svAction);
            DrawText(strAction.c_str(), static_cast<int>(tCap.x), static_cast<int>(tCap.y + tCap.height + 6.0f), 16,
                     Fade(kHintColor, 0.8f));
        }
    }
}

void DrawPlayer(const Entity& tPlayer)
{
    const Rectangle tRect = ToRaylib(GetBounds(tPlayer.tBody));
    const DashState& tDash = tPlayer.tMotor.tDash;
    Color tColor = kPlayerColor;
    if (tDash.IsDashing())
        tColor = kPlayerDashColor;
    else if (tDash.bRunning)
        tColor = kPlayerRunColor;
    if (tPlayer.tHit.fFlashTimer > 0.0f)
        tColor = kHitFlashColor;
    if (tDash.fInvulnTimer > 0.0f)
        tColor = Fade(tColor, 0.5f);  // 무적 중 반투명
    DrawRectangleRec(tRect, tColor);
    if (tPlayer.tBody.bGrounded)
        DrawRectangleLinesEx(tRect, 2.0f, kPlayerGroundedOutline);

    // 머리 위 대시 게이지: 꽉 차 있고 안 쓰는 중이면 숨김. 흰 눈금 = 대시 가능선(50%)
    if (tDash.fGauge < 1.0f || tDash.bRunning || tDash.IsDashing())
    {
        constexpr float kBarW = 32.0f;
        constexpr float kBarH = 4.0f;
        const float fX = tRect.x + (tRect.width - kBarW) * 0.5f;
        const float fY = tRect.y - 10.0f;
        DrawRectangleRec(Rectangle{fX - 1.0f, fY - 1.0f, kBarW + 2.0f, kBarH + 2.0f}, kHpBarBackColor);
        DrawRectangleRec(Rectangle{fX, fY, kBarW * tDash.fGauge, kBarH},
                         tDash.bExhausted ? kGaugeExhaustedColor : kPlayerRunColor);
        DrawRectangleRec(Rectangle{fX + kBarW * kPlayerDef.fDashCost - 0.5f, fY - 1.0f, 1.0f, kBarH + 2.0f},
                         kPlayerGroundedOutline);
    }
}

void DrawEnemy(const Entity& tEnemy)
{
    const Rectangle tRect = ToRaylib(GetBounds(tEnemy.tBody));
    DrawRectangleRec(tRect, tEnemy.tHit.fFlashTimer > 0.0f ? kHitFlashColor : kEnemyColor);

    // 머리 위: 남은 타격 수만큼 점 (1·2·3대). 글자 없이 몇 대 남았는지 보이게
    const int iHitsLeft = static_cast<int>(std::ceil(tEnemy.fHp / kPlayerDef.fAttackDamage - 0.001f));
    constexpr float kPip = 4.0f;
    constexpr float kPipGap = 3.0f;
    const float fTotalW = static_cast<float>(iHitsLeft) * kPip + static_cast<float>(iHitsLeft - 1) * kPipGap;
    float fPipX = tRect.x + (tRect.width - fTotalW) * 0.5f;
    for (int i = 0; i < iHitsLeft; ++i)
    {
        DrawRectangleRec(Rectangle{fPipX - 1.0f, tRect.y - 9.0f, kPip + 2.0f, kPip + 2.0f}, kHpBarBackColor);
        DrawRectangleRec(Rectangle{fPipX, tRect.y - 8.0f, kPip, kPip}, kHpBarColor);
        fPipX += kPip + kPipGap;
    }
}

void DrawParticles(const Particles& tParticles)
{
    for (const Particle& tParticle : tParticles.GetAll())
    {
        if (tParticle.fLife <= 0.0f)
            continue;
        const float fRatio = tParticle.fLife / tParticle.fMaxLife;            // 1 -> 0
        const float fSize = tParticle.fSize * (1.0f + 2.0f * fRatio) / 3.0f;  // 시작 크기 -> 1/3
        const Rect rcSpark = {tParticle.vPos.fX - fSize * 0.5f, tParticle.vPos.fY - fSize * 0.5f, fSize, fSize};
        DrawRectangleRec(ToRaylib(rcSpark), Fade(kSparkColor, fRatio));
    }
}

// 대시 잔상: 최근 것일수록 진하고, 사라지며 살짝 줄어듦
void DrawGhosts(const Particles& tParticles)
{
    for (const Ghost& tGhost : tParticles.GetGhosts())
    {
        if (tGhost.fLife <= 0.0f)
            continue;
        const float fRatio = tGhost.fLife / Particles::kGhostLife;  // 1 -> 0
        const float fShrink = (1.0f - fRatio) * 4.0f;
        const Rect rcBody = tGhost.rcBody;
        const Rectangle tRect = {rcBody.fX + fShrink * 0.5f, rcBody.fY + fShrink, rcBody.fW - fShrink,
                                 rcBody.fH - fShrink};
        DrawRectangleRec(tRect, Fade(kPlayerDashColor, 0.45f * fRatio));
    }
}

void DrawAttack(const AttackState& tAttack)
{
    if (tAttack.ePhase == AttackPhase::Active)
        DrawRectangleRec(ToRaylib(tAttack.rcHitbox), kHitboxColor);
}

void DrawDebug(const Entity& tPlayerEntity, int iAliveCount, const DebugInfo& tDebug)
{
    const Body& tPlayer = tPlayerEntity.tBody;
    const PlayerMotorState& tMotor = tPlayerEntity.tMotor;
    constexpr int kX = 24;
    constexpr int kLineHeight = 20;
    constexpr int kFontSize = 18;
    int iY = kFrameOffsetY + 70;

    DrawText(TextFormat("FPS %d  steps %d  ticks %lld  entities %d", GetFPS(), tDebug.iStepsThisFrame,
                        tDebug.llTotalTicks, iAliveCount),
             kX, iY, kFontSize, kDebugTextColor);
    iY += kLineHeight;
    DrawText(TextFormat("pos (%.0f, %.0f)  vel (%.0f, %.0f)  grounded %d", tPlayer.vPos.fX, tPlayer.vPos.fY,
                        tPlayer.vVel.fX, tPlayer.vVel.fY, tPlayer.bGrounded ? 1 : 0),
             kX, iY, kFontSize, kDebugTextColor);
    iY += kLineHeight;
    DrawText(TextFormat("gauge %.2f%s  dash %.2f  invuln %.2f  run %d", tMotor.tDash.fGauge,
                        tMotor.tDash.bExhausted ? " EXHAUSTED" : "", tMotor.tDash.fDashTimer,
                        tMotor.tDash.fInvulnTimer, tMotor.tDash.bRunning ? 1 : 0),
             kX, iY, kFontSize, kDebugTextColor);
    iY += kLineHeight;
    // 입력 확인용: 키보드에 따라 Shift + Z + 방향키 동시 입력이 안 먹을 수 있음(키 고스팅)
    DrawText(TextFormat("input: dir %d  shift %d  hold %.2f  jump %d", tPlayerEntity.tMoveIntent.iMoveDir,
                        tPlayerEntity.tMoveIntent.bDashHeld ? 1 : 0, tMotor.tDash.fHoldTimer,
                        tPlayerEntity.tMoveIntent.bJumpHeld ? 1 : 0),
             kX, iY, kFontSize, kDebugTextColor);
}
} // namespace

void Render::Draw(const FrameView& tView) const
{
    BeginDrawing();
    ClearBackground(kFrameColor);

    const Vec2 vTopLeft = tView.tCamera.GetTopLeft();
    Camera2D tCamera = {};
    tCamera.target = Vector2{vTopLeft.fX, vTopLeft.fY};
    tCamera.offset = Vector2{0.0f, static_cast<float>(kFrameOffsetY)};
    tCamera.zoom = 1.0f;

    // 방 영역 밖(위·아래 프레임)엔 안 그려지게 자름
    BeginScissorMode(0, kFrameOffsetY, kScreenWidth, kViewHeight);
    BeginMode2D(tCamera);
    {
        DrawRoom(tView.tRoom, vTopLeft);
        DrawDoor(tView.tRoomFlow);
        DrawHints(tView.tRoomFlow.GetRoomDef());

        tView.tWorld.ForEachAlive([](Handle, const Entity& tEntity) {  // const 순회: Render는 World를 못 바꿈
            if (tEntity.eType == EntityType::Enemy)
                DrawEnemy(tEntity);
        });
        DrawGhosts(tView.tParticles);  // 플레이어보다 먼저 = 뒤에 깔림
        if (const Entity* pPlayer = tView.tWorld.Get(tView.hPlayer))
        {
            DrawPlayer(*pPlayer);
            DrawAttack(pPlayer->tAttack);
        }
        DrawParticles(tView.tParticles);
    }
    EndMode2D();
    EndScissorMode();

    // 화면 고정 UI
    DrawHud(tView);
    if (tView.tDebug.bShow)
        if (const Entity* pPlayer = tView.tWorld.Get(tView.hPlayer))
            DrawDebug(*pPlayer, tView.tWorld.GetAliveCount(), tView.tDebug);

    EndDrawing();
}
