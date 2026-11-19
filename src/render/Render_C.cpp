#include "render/Render_C.h"

#include "core/Body_C.h"
#include "core/Profiler_C.h"
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
#include "systems/Items_C.h"
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
constexpr Color kDummyColor = {140, 110, 120, 255};   // 튜토리얼 허수아비 (공격 안 함)
constexpr Color kShooterColor = {60, 190, 180, 255};   // Shooter (청록)
constexpr Color kBulletColor = {255, 150, 60, 255};    // 적 탄 (주황)
constexpr Color kBulletCoreColor = {255, 235, 180, 255};
constexpr Color kWindupColor = {255, 160, 50, 255};   // 공격 예고
constexpr Color kBossBodyColor = {150, 80, 210, 255};    // 보스 (보라)
constexpr Color kBossPhase2Color = {190, 60, 150, 255};  // 보스 2페이즈 (붉은 보라)
constexpr Color kBossEyeColor = {250, 240, 220, 255};
constexpr Color kHitFlashColor = {255, 255, 255, 255};
constexpr Color kHpBarColor = {120, 220, 120, 255};
constexpr Color kHpBarBackColor = {40, 40, 40, 255};
constexpr Color kSparkColor = {255, 255, 255, 255};
constexpr Color kDoorLockedColor = {170, 50, 60, 255};
constexpr Color kDoorOpenColor = {90, 220, 140, 255};
constexpr Color kHintColor = {230, 230, 240, 200};
constexpr Color kDebugTextColor = {220, 220, 220, 255};
constexpr Color kMeasureTextColor = {120, 230, 160, 255};  // 측정 줄 (초록)
constexpr float kCullMargin = 48.0f;  // 컬링 여백 (머리 위 점·드랍 이름표가 잘리지 않게)

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
        tColor = Fade(tColor, 0.5f);  // 대시 무적 중 반투명
    else if (tPlayer.tHit.fInvulnTimer > 0.0f && static_cast<int>(tPlayer.tHit.fInvulnTimer / 0.08f) % 2 == 0)
        tColor = Fade(tColor, 0.25f);  // 피격 무적: 깜빡임 (안 맞는 시간이 보이게)
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

// 근접 적 공격 예고: Windup 동안만 실제 히트박스 자리를 그림. 시간이 갈수록 진해짐 (11/19 사용자 요청)
void DrawEnemyWindupRange(const Entity& tEnemy)
{
    if (tEnemy.eType != EntityType::Enemy || tEnemy.tAI.eState != EnemyState::Windup)
        return;
    const float fProgress = std::clamp(1.0f - tEnemy.tAI.fTimer / kChaserDef.fWindupTime, 0.0f, 1.0f);
    const Rectangle tRange =
        ToRaylib(ComputeAttackHitbox(tEnemy.tBody, tEnemy.tMotor.iFacing, kChaserDef.tAttack.vSize));
    DrawRectangleRec(tRange, Fade(kWindupColor, 0.15f + 0.35f * fProgress));
    DrawRectangleLinesEx(tRange, 1.0f, Fade(kWindupColor, 0.9f));
}

void DrawEnemy(const Entity& tEnemy, float fPlayerDamage)
{
    DrawEnemyWindupRange(tEnemy);  // 몸보다 먼저 = 몸 뒤에 깔림

    const Rectangle tRect = ToRaylib(GetBounds(tEnemy.tBody));
    Color tColor = tEnemy.tAI.bPassive ? kDummyColor : kEnemyColor;
    if (tEnemy.eType == EntityType::Shooter)
        tColor = kShooterColor;
    // 공격 예고(Windup): 주황/빨강 0.05초마다 번갈아 -> 곧 때린다는 신호
    if (tEnemy.tAI.eState == EnemyState::Windup && static_cast<int>(tEnemy.tAI.fTimer / 0.05f) % 2 == 0)
        tColor = kWindupColor;
    if (tEnemy.tHit.fFlashTimer > 0.0f)
        tColor = kHitFlashColor;
    DrawRectangleRec(tRect, tColor);

    // 머리 위: 남은 타격 수만큼 점 (1·2·3대). 글자 없이 몇 대 남았는지 보이게
    const int iHitsLeft = static_cast<int>(std::ceil(tEnemy.fHp / fPlayerDamage - 0.001f));
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

// 보스: 큰 보라 사각형 + 눈(바라보는 쪽). 예고(Windup) 중 깜빡임, 2페이즈는 붉은 보라 (HP는 화면 위 보스 바)
void DrawBoss(const Entity& tBoss)
{
    const Rectangle tRect = ToRaylib(GetBounds(tBoss.tBody));
    const bool bPhase2 = tBoss.fHp <= kBossDef.fMaxHp * kBossDef.fPhase2HpRatio;
    Color tColor = bPhase2 ? kBossPhase2Color : kBossBodyColor;
    const BossState eState = tBoss.tBoss.eState;
    const bool bWindup = eState == BossState::AimedWindup || eState == BossState::RingWindup;
    if (bWindup && static_cast<int>(tBoss.tBoss.fTimer / 0.06f) % 2 == 0)
        tColor = kWindupColor;
    if (tBoss.tHit.fFlashTimer > 0.0f)
        tColor = kHitFlashColor;
    DrawRectangleRec(tRect, tColor);
    DrawRectangleLinesEx(tRect, 3.0f, Fade(BLACK, 0.35f));

    // 눈 2개: 바라보는 쪽으로 치우침 (iFacing은 Motor가 이동 의도로 정함)
    const float fLook = static_cast<float>(tBoss.tMotor.iFacing) * 8.0f;
    const float fEyeY = tRect.y + tRect.height * 0.32f;
    for (const float fOffset : {-12.0f, 12.0f})
        DrawRectangleRec(Rectangle{tRect.x + tRect.width * 0.5f + fOffset + fLook - 4.0f, fEyeY, 8.0f, 10.0f},
                         kBossEyeColor);
    // 원형탄 예고는 몸 둘레에 고리 (조준 예고와 구분)
    if (eState == BossState::RingWindup)
        DrawCircleLinesV(Vector2{tRect.x + tRect.width * 0.5f, tRect.y + tRect.height * 0.5f},
                         tRect.width * (0.9f + 0.3f * tBoss.tBoss.fTimer), Fade(kWindupColor, 0.8f));
}

// 투사체: 주황 원 + 밝은 심
void DrawProjectile(const Entity& tBullet)
{
    const Rect rcBody = GetBounds(tBullet.tBody);
    const Vector2 vCenter = {rcBody.fX + rcBody.fW * 0.5f, rcBody.fY + rcBody.fH * 0.5f};
    DrawCircleV(vCenter, rcBody.fW * 0.6f, Fade(kBulletColor, 0.35f));
    DrawCircleV(vCenter, rcBody.fW * 0.45f, kBulletColor);
    DrawCircleV(vCenter, rcBody.fW * 0.2f, kBulletCoreColor);
}

// 바닥 아이템: 아이템 색 마름모가 둥실 + 위에 이름
void DrawDrop(const Entity& tDrop, const DropEntry& tEntry)
{
    const ItemDef& tDef = GetItemDef(tEntry.eItem);
    const Color tColor = {static_cast<unsigned char>(tDef.tColor.iR), static_cast<unsigned char>(tDef.tColor.iG),
                          static_cast<unsigned char>(tDef.tColor.iB), 255};
    const Rect rcBody = GetBounds(tDrop.tBody);
    const float fBob = tDrop.tBody.bGrounded ? std::sin(static_cast<float>(GetTime()) * 4.0f) * 3.0f - 4.0f : 0.0f;
    const Vector2 vCenter = {rcBody.fX + rcBody.fW * 0.5f, rcBody.fY + rcBody.fH * 0.5f + fBob};
    DrawPoly(vCenter, 4, rcBody.fW * 0.75f, 0.0f, tColor);
    DrawPolyLines(vCenter, 4, rcBody.fW * 0.75f, 0.0f, WHITE);
    const std::string strName(tDef.svName);
    constexpr int kFontSize = 14;
    DrawText(strName.c_str(), static_cast<int>(vCenter.x) - MeasureText(strName.c_str(), kFontSize) / 2,
             static_cast<int>(vCenter.y - rcBody.fH - 14.0f), kFontSize, Fade(tColor, 0.9f));
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
    iY += kLineHeight + 8;

    // 측정 (measure.md): 구간별 ms (프레임당 합, 평균) + 지난 틱 검색 비용
    if (tDebug.pProfiler != nullptr)
    {
        const FrameProfiler& tProfiler = *tDebug.pProfiler;
        // 틱당 평균 (프레임당 틱 수에 안 흔들림). render만 프레임당
        auto fnMs = [&](ProfileSection eSection) { return tProfiler.GetSmoothedPerCallMs(eSection); };
        DrawText(TextFormat("ms/tick  tick %.3f = ai %.3f  physics %.3f  proj %.3f  spatial+combat %.3f",
                            fnMs(ProfileSection::Tick), fnMs(ProfileSection::Ai), fnMs(ProfileSection::Physics),
                            fnMs(ProfileSection::Projectiles), fnMs(ProfileSection::SpatialCombat)),
                 kX, iY, kFontSize, kMeasureTextColor);
        iY += kLineHeight;
        DrawText(TextFormat("ms/frame render %.2f", fnMs(ProfileSection::Render)), kX, iY, kFontSize,
                 kMeasureTextColor);
        iY += kLineHeight;
    }
    const SpatialQueryStats& tStats = tDebug.tSpatialStats;
    DrawText(TextFormat("spatial [%s]  queries %d  slots %lld  overlap checks %lld   (F5 = record 3s -> csv)",
                        tDebug.szSpatialMode, tStats.iQueries, tStats.llSlotsVisited, tStats.llOverlapChecks),
             kX, iY, kFontSize, kMeasureTextColor);
}
} // namespace

void Render::Draw(const FrameView& tView) const
{
    BeginDrawing();
    ClearBackground(kFrameColor);

    const Vec2 vTopLeft = tView.tCamera.GetTopLeft();
    Camera2D tCamera = {};
    const Vec2 vShake = tView.tCamera.GetShakeOffset();  // 피격 흔들림은 그리기에만 (판정·컬링 무관)
    tCamera.target = Vector2{vTopLeft.fX + vShake.fX, vTopLeft.fY + vShake.fY};
    tCamera.offset = Vector2{0.0f, static_cast<float>(kFrameOffsetY)};
    tCamera.zoom = 1.0f;

    // 방 영역 밖(위·아래 프레임)엔 안 그려지게 자름
    BeginScissorMode(0, kFrameOffsetY, kScreenWidth, kViewHeight);
    BeginMode2D(tCamera);
    {
        DrawRoom(tView.tRoom, vTopLeft);
        DrawDoor(tView.tRoomFlow);
        DrawHints(tView.tRoomFlow.GetRoomDef());

        // 컬링: 화면(+여백) 밖 몸은 안 그림 (11/19 측정 후 추가). 머리 위 점·이름표가 있어서 여백을 둠
        const Rect rcVisible = {vTopLeft.fX - kCullMargin, vTopLeft.fY - kCullMargin, kViewSize.fX + kCullMargin * 2.0f,
                                kViewSize.fY + kCullMargin * 2.0f};
        tView.tWorld.ForEachAlive([&](Handle hEntity, const Entity& tEntity) {  // const 순회: Render는 World를 못 바꿈
            if (!Overlaps(rcVisible, GetBounds(tEntity.tBody)))
                return;
            if (tEntity.eType == EntityType::Enemy || tEntity.eType == EntityType::Shooter)
                DrawEnemy(tEntity, tView.tPlayerStats.fAttackDamage);
            else if (tEntity.eType == EntityType::Boss)
                DrawBoss(tEntity);
            else if (tEntity.eType == EntityType::Drop)
                if (const DropEntry* pEntry = tView.tItems.FindDrop(hEntity))
                    DrawDrop(tEntity, *pEntry);
        });
        DrawGhosts(tView.tParticles);  // 플레이어보다 먼저 = 뒤에 깔림
        if (const Entity* pPlayer = tView.tWorld.Get(tView.hPlayer))
        {
            DrawPlayer(*pPlayer);
            DrawAttack(pPlayer->tAttack);
        }
        tView.tWorld.ForEachAlive([&](Handle, const Entity& tEntity) {  // 탄은 맨 위 (잘 보이게)
            if (tEntity.eType == EntityType::Projectile && Overlaps(rcVisible, GetBounds(tEntity.tBody)))
                DrawProjectile(tEntity);
        });
        DrawParticles(tView.tParticles);
    }
    EndMode2D();
    EndScissorMode();

    // 화면 고정 UI
    DrawHud(tView);
    if (tView.tDebug.bShow)
        if (const Entity* pPlayer = tView.tWorld.Get(tView.hPlayer))
            DrawDebug(*pPlayer, tView.tWorld.GetAliveCount(), tView.tDebug);
}

void Render::Present() const
{
    EndDrawing();
}
