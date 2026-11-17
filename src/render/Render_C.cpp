#include "render/Render_C.h"

#include "core/Body_C.h"
#include "core/Rect_C.h"
#include "core/Tilemap_C.h"
#include "core/World_U.h"
#include "data/EnemyDef_C.h"
#include "render/Particles_C.h"
#include "systems/Combat_C.h"
#include "systems/PlayerMotor_C.h"

#include "raylib.h"

#include <algorithm>

namespace
{
constexpr int kTilePx = static_cast<int>(kTileSize);
// 720 - 22 * 32 = 16px -> 위·아래 8px씩 검은 프레임
constexpr int kFrameOffsetY = (kScreenHeight - kRoomRows * kTilePx) / 2;

constexpr Color kFrameColor = {0, 0, 0, 255};
constexpr Color kRoomBackground = {24, 20, 37, 255};
constexpr Color kSolidColor = {88, 80, 110, 255};
constexpr Color kPlayerColor = {232, 196, 92, 255};
constexpr Color kPlayerDashColor = {110, 210, 240, 255};
constexpr Color kPlayerGroundedOutline = {255, 255, 255, 255};
constexpr Color kHitboxColor = {240, 80, 80, 140};
constexpr Color kEnemyColor = {200, 70, 90, 255};
constexpr Color kHitFlashColor = {255, 255, 255, 255};
constexpr Color kHpBarColor = {120, 220, 120, 255};
constexpr Color kHpBarBackColor = {40, 40, 40, 255};
constexpr Color kSparkColor = {255, 255, 255, 255};
constexpr Color kDebugTextColor = {220, 220, 220, 255};

// raylib의 Rectangle은 외부 타입이라 필드명(x, y, width, height)이 raylib 것 그대로임
Rectangle ToScreen(const Rect& rcArea)
{
    return Rectangle{rcArea.fX, rcArea.fY + static_cast<float>(kFrameOffsetY), rcArea.fW, rcArea.fH};
}

void DrawRoom(const Tilemap& tRoom)
{
    DrawRectangle(0, kFrameOffsetY, tRoom.GetCols() * kTilePx, tRoom.GetRows() * kTilePx, kRoomBackground);
    for (int iTileY = 0; iTileY < tRoom.GetRows(); ++iTileY)
    {
        for (int iTileX = 0; iTileX < tRoom.GetCols(); ++iTileX)
        {
            if (tRoom.IsSolid(iTileX, iTileY))
                DrawRectangle(iTileX * kTilePx, iTileY * kTilePx + kFrameOffsetY, kTilePx, kTilePx, kSolidColor);
        }
    }
}

void DrawPlayer(const Entity& tPlayer)
{
    const Rectangle tScreenRect = ToScreen(GetBounds(tPlayer.tBody));
    Color tColor = tPlayer.tMotor.tDash.bActive ? kPlayerDashColor : kPlayerColor;
    if (tPlayer.tHit.fFlashTimer > 0.0f)
        tColor = kHitFlashColor;
    DrawRectangleRec(tScreenRect, tColor);
    if (tPlayer.tBody.bGrounded)
        DrawRectangleLinesEx(tScreenRect, 2.0f, kPlayerGroundedOutline);
}

void DrawEnemy(const Entity& tEnemy)
{
    const Rectangle tScreenRect = ToScreen(GetBounds(tEnemy.tBody));
    DrawRectangleRec(tScreenRect, tEnemy.tHit.fFlashTimer > 0.0f ? kHitFlashColor : kEnemyColor);

    // 머리 위 HP 바
    const float fRatio = std::clamp(tEnemy.fHp / kChaserDef.fMaxHp, 0.0f, 1.0f);
    const Rectangle tBarBack = {tScreenRect.x, tScreenRect.y - 6.0f, tScreenRect.width, 3.0f};
    DrawRectangleRec(tBarBack, kHpBarBackColor);
    DrawRectangleRec(Rectangle{tBarBack.x, tBarBack.y, tBarBack.width * fRatio, tBarBack.height}, kHpBarColor);
}

void DrawParticles(const Particles& tParticles)
{
    for (const Particle& tParticle : tParticles.GetAll())
    {
        if (tParticle.fLife <= 0.0f)
            continue;
        const float fRatio = tParticle.fLife / tParticle.fMaxLife;  // 1 -> 0
        const float fSize = tParticle.fSize * (1.0f + 2.0f * fRatio) / 3.0f;  // 시작 크기 -> 1/3
        const Rect rcSpark = {tParticle.vPos.fX - fSize * 0.5f, tParticle.vPos.fY - fSize * 0.5f, fSize, fSize};
        DrawRectangleRec(ToScreen(rcSpark), Fade(kSparkColor, fRatio));
    }
}

void DrawAttack(const AttackState& tAttack)
{
    if (tAttack.ePhase == AttackPhase::Active)
        DrawRectangleRec(ToScreen(tAttack.rcHitbox), kHitboxColor);
}

const char* ToString(AttackPhase ePhase)
{
    switch (ePhase)
    {
    case AttackPhase::Ready: return "Ready";
    case AttackPhase::Active: return "Active";
    case AttackPhase::Recover: return "Recover";
    }
    return "?";
}

void DrawDebug(const Entity& tPlayerEntity, int iAliveCount, const DebugInfo& tDebug)
{
    const Body& tPlayer = tPlayerEntity.tBody;
    const PlayerMotorState& tMotor = tPlayerEntity.tMotor;
    const AttackState& tAttack = tPlayerEntity.tAttack;
    constexpr int kX = 40;
    constexpr int kLineHeight = 22;
    constexpr int kFontSize = 20;
    int iY = kFrameOffsetY + kTilePx + 8;

    DrawText(TextFormat("FPS: %d   updates this frame: %d   total ticks: %lld", GetFPS(), tDebug.iStepsThisFrame,
                        tDebug.llTotalTicks),
             kX, iY, kFontSize, kDebugTextColor);
    iY += kLineHeight;
    DrawText(TextFormat("pos: (%.1f, %.1f)   vel: (%.1f, %.1f)   grounded: %s", tPlayer.vPos.fX, tPlayer.vPos.fY,
                        tPlayer.vVel.fX, tPlayer.vVel.fY, tPlayer.bGrounded ? "yes" : "no"),
             kX, iY, kFontSize, kDebugTextColor);
    iY += kLineHeight;
    DrawText(TextFormat("coyote: %.3f   jump buffer: %.3f", tMotor.tJump.fCoyoteTimer, tMotor.tJump.fBufferTimer), kX,
             iY, kFontSize, kDebugTextColor);
    iY += kLineHeight;
    DrawText(TextFormat("dash gauge: %.2f %s   attack: %s", tMotor.tDash.fGauge,
                        tMotor.tDash.bExhausted ? "(EXHAUSTED)" : "", ToString(tAttack.ePhase)),
             kX, iY, kFontSize, kDebugTextColor);
    iY += kLineHeight;
    DrawText(TextFormat("player hp: %.0f   entities: %d", tPlayerEntity.fHp, iAliveCount), kX, iY, kFontSize,
             kDebugTextColor);
}
} // namespace

void Render::Draw(const Tilemap& tRoom, const World& tWorld, Handle hPlayer, const Particles& tParticles,
                  const DebugInfo& tDebug) const
{
    BeginDrawing();
    ClearBackground(kFrameColor);

    DrawRoom(tRoom);
    tWorld.ForEachAlive([](Handle, const Entity& tEntity) {  // const 순회: Render는 World를 못 바꿈
        if (tEntity.eType == EntityType::Enemy)
            DrawEnemy(tEntity);
    });
    if (const Entity* pPlayer = tWorld.Get(hPlayer))
    {
        DrawPlayer(*pPlayer);
        DrawAttack(pPlayer->tAttack);
    }
    DrawParticles(tParticles);
    if (const Entity* pPlayer = tWorld.Get(hPlayer))
        DrawDebug(*pPlayer, tWorld.GetAliveCount(), tDebug);

    EndDrawing();
}
