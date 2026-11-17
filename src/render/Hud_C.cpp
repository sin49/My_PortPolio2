#include "render/Hud_C.h"

#include "core/Tilemap_C.h"
#include "core/World_U.h"
#include "data/EnemyDef_C.h"
#include "data/PlayerDef_C.h"
#include "data/Rooms_C.h"
#include "render/Render_C.h"
#include "render/ViewCamera_C.h"
#include "systems/PlayerMotor_C.h"
#include "systems/RoomFlow_C.h"

#include "raylib.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr Color kPanelColor = {0, 0, 0, 150};
constexpr Color kHpColor = {225, 70, 80, 255};
constexpr Color kGaugeColor = {110, 210, 240, 255};
constexpr Color kGaugeExhaustedColor = {90, 90, 110, 255};
constexpr Color kBarBackColor = {35, 32, 45, 255};
constexpr Color kMarkColor = {255, 255, 255, 180};
constexpr Color kDotColor = {200, 200, 210, 255};
constexpr Color kBossColor = {170, 90, 220, 255};
constexpr Color kArrowColor = {90, 220, 140, 255};
constexpr Color kTitleColor = {240, 230, 200, 255};
constexpr Color kPlayerDotColor = {232, 196, 92, 255};

constexpr int kMargin = 16;
constexpr int kTop = kFrameOffsetY + kMargin;

// 0~1 비율 바
void DrawBar(int iX, int iY, int iW, int iH, float fRatio, Color tFill)
{
    DrawRectangle(iX - 2, iY - 2, iW + 4, iH + 4, kPanelColor);
    DrawRectangle(iX, iY, iW, iH, kBarBackColor);
    DrawRectangle(iX, iY, static_cast<int>(static_cast<float>(iW) * std::clamp(fRatio, 0.0f, 1.0f)), iH, tFill);
}

void DrawPlayerBars(const Entity& tPlayer)
{
    constexpr int kBarW = 260;
    DrawBar(kMargin, kTop, kBarW, 16, tPlayer.fHp / kPlayerDef.fMaxHp, kHpColor);

    const DashState& tDash = tPlayer.tMotor.tDash;
    DrawBar(kMargin, kTop + 24, kBarW, 8, tDash.fGauge, tDash.bExhausted ? kGaugeExhaustedColor : kGaugeColor);
    // 대시 비용(50%) 눈금: 이 선을 넘어야 대시 가능
    const int iMarkX = kMargin + static_cast<int>(static_cast<float>(kBarW) * kPlayerDef.fDashCost);
    DrawRectangle(iMarkX - 1, kTop + 22, 2, 12, kMarkColor);
}

// 오른쪽 위: 방 진행 점 (지금 방 = 채움, 보스 = 마름모)
void DrawRoomProgress(const RoomFlow& tRoomFlow, int iY)
{
    const int iCount = static_cast<int>(kRooms.size());
    constexpr int kGap = 26;
    const int iStartX = kScreenWidth - kMargin - 10 - (iCount - 1) * kGap;
    for (int i = 0; i < iCount; ++i)
    {
        const Vector2 vCenter = {static_cast<float>(iStartX + i * kGap), static_cast<float>(iY + 10)};
        const bool bCurrent = i == tRoomFlow.GetRoomIndex();
        const bool bDone = i < tRoomFlow.GetRoomIndex();
        const Color tColor = kRooms[static_cast<std::size_t>(i)].bBossRoom ? kBossColor : kDotColor;
        if (kRooms[static_cast<std::size_t>(i)].bBossRoom)
            DrawPoly(vCenter, 4, bCurrent ? 10.0f : 8.0f, 45.0f, Fade(tColor, bCurrent || bDone ? 1.0f : 0.35f));
        else if (bCurrent || bDone)
            DrawCircleV(vCenter, bCurrent ? 8.0f : 6.0f, Fade(tColor, bCurrent ? 1.0f : 0.6f));
        else
            DrawCircleLinesV(vCenter, 6.0f, tColor);
    }
}

int CountEnemiesLeft(const World& tWorld)
{
    int iCount = 0;
    tWorld.ForEachAlive([&](Handle, const Entity& tEntity) {
        if (tEntity.eType == EntityType::Enemy && tEntity.fHp > 0.0f)  // TODO: Shooter·Boss 추가 후 포함
            ++iCount;
    });
    return iCount;
}

// 오른쪽 위 진행 점 아래: 남은 적 = 빨간 칸, 처치한 적 = 빈 칸 (글자 없음)
void DrawEnemyCounter(int iLeft, int iTotal, int iY)
{
    constexpr int kCell = 8;
    constexpr int kGap = 3;
    const int iW = iTotal * kCell + (iTotal - 1) * kGap;
    int iX = kScreenWidth - kMargin - iW;
    for (int i = 0; i < iTotal; ++i)
    {
        if (i < iLeft)
            DrawRectangle(iX, iY, kCell, kCell, kHpColor);
        else
            DrawRectangleLines(iX, iY, kCell, kCell, Fade(kDotColor, 0.5f));
        iX += kCell + kGap;
    }
}

// 남은 적이 처음의 1/4 이하가 되면: 화면 밖 적마다 화면 가장자리에 그쪽을 가리키는 화살표
void DrawOffscreenEnemyArrows(const World& tWorld, Vec2 vCameraTopLeft)
{
    constexpr float kEdge = 28.0f;
    const Vector2 vCenter = {kViewSize.fX * 0.5f, static_cast<float>(kFrameOffsetY) + kViewSize.fY * 0.5f};
    const float fBlink = 0.6f + 0.4f * std::sin(static_cast<float>(GetTime()) * 5.0f);

    tWorld.ForEachAlive([&](Handle, const Entity& tEntity) {
        if (tEntity.eType != EntityType::Enemy || tEntity.fHp <= 0.0f)
            return;
        const Body& tBody = tEntity.tBody;
        const Vector2 vScreen = {tBody.vPos.fX + tBody.vSize.fX * 0.5f - vCameraTopLeft.fX,
                                 tBody.vPos.fY + tBody.vSize.fY * 0.5f - vCameraTopLeft.fY +
                                     static_cast<float>(kFrameOffsetY)};
        const bool bOnScreen = vScreen.x >= 0.0f && vScreen.x <= kViewSize.fX &&
                               vScreen.y >= static_cast<float>(kFrameOffsetY) &&
                               vScreen.y <= static_cast<float>(kFrameOffsetY) + kViewSize.fY;
        if (bOnScreen)
            return;

        // 화면 가운데 -> 적 방향으로 가다가 가장자리(여백 kEdge)에 닿는 점
        const float fDx = vScreen.x - vCenter.x;
        const float fDy = vScreen.y - vCenter.y;
        const float fHalfW = kViewSize.fX * 0.5f - kEdge;
        const float fHalfH = kViewSize.fY * 0.5f - kEdge;
        const float fScale = std::min(fHalfW / std::max(std::abs(fDx), 0.001f), fHalfH / std::max(std::abs(fDy), 0.001f));
        const Vector2 vTip = {vCenter.x + fDx * fScale, vCenter.y + fDy * fScale};

        const float fLen = std::sqrt(fDx * fDx + fDy * fDy);
        const Vector2 vDir = {fDx / fLen, fDy / fLen};
        const Vector2 vSide = {-vDir.y, vDir.x};
        constexpr float kLength = 18.0f;
        constexpr float kHalfWidth = 9.0f;
        const Vector2 vBase = {vTip.x - vDir.x * kLength, vTip.y - vDir.y * kLength};
        const Vector2 vLeft = {vBase.x + vSide.x * kHalfWidth, vBase.y + vSide.y * kHalfWidth};
        const Vector2 vRight = {vBase.x - vSide.x * kHalfWidth, vBase.y - vSide.y * kHalfWidth};
        // raylib 삼각형은 반시계 순서여야 보임 -> 양쪽 순서로 그려서 방향 상관없이 보이게
        DrawTriangle(vTip, vLeft, vRight, Fade(kHpColor, fBlink));
        DrawTriangle(vTip, vRight, vLeft, Fade(kHpColor, fBlink));
    });
}

// 오른쪽 위 미니맵 (반환: 미니맵 아래 y): 벽 = 회색, 문 = 빨강/초록, 적 = 빨간 점, 플레이어 = 노란 점, 흰 테두리 = 지금 화면
int DrawMinimap(const FrameView& tView)
{
    const Tilemap& tRoom = tView.tRoom;
    constexpr float kMaxW = 240.0f;
    constexpr float kMaxH = 84.0f;
    const float fScale = std::min(kMaxW / static_cast<float>(tRoom.GetCols()), kMaxH / static_cast<float>(tRoom.GetRows()));
    const float fW = fScale * static_cast<float>(tRoom.GetCols());
    const float fH = fScale * static_cast<float>(tRoom.GetRows());
    const float fX0 = static_cast<float>(kScreenWidth - kMargin) - fW;
    const float fY0 = static_cast<float>(kTop);
    const float fPxToMap = fScale / kTileSize;  // 방 px -> 미니맵 px

    DrawRectangleRec(Rectangle{fX0 - 4.0f, fY0 - 4.0f, fW + 8.0f, fH + 8.0f}, kPanelColor);
    for (int iY = 0; iY < tRoom.GetRows(); ++iY)
        for (int iX = 0; iX < tRoom.GetCols(); ++iX)
            if (tRoom.IsSolid(iX, iY))
                DrawRectangleRec(Rectangle{fX0 + static_cast<float>(iX) * fScale, fY0 + static_cast<float>(iY) * fScale,
                                           fScale + 0.5f, fScale + 0.5f},
                                 Fade(kDotColor, 0.35f));

    const RoomFlow& tRoomFlow = tView.tRoomFlow;
    if (!tRoomFlow.GetRoomDef().bBossRoom)
    {
        const Rect rcDoor = GetRoomDoor(tRoomFlow.GetRoomDef());
        const bool bOpen = tRoomFlow.GetRoomState() == RoomState::Cleared;
        DrawRectangleRec(Rectangle{fX0 + rcDoor.fX * fPxToMap, fY0 + rcDoor.fY * fPxToMap, rcDoor.fW * fPxToMap,
                                   rcDoor.fH * fPxToMap},
                         bOpen ? kArrowColor : kHpColor);
    }

    auto fnDot = [&](const Body& tBody, float fRadius, Color tColor) {
        const Vector2 vCenter = {fX0 + (tBody.vPos.fX + tBody.vSize.fX * 0.5f) * fPxToMap,
                                 fY0 + (tBody.vPos.fY + tBody.vSize.fY * 0.5f) * fPxToMap};
        DrawCircleV(vCenter, fRadius, tColor);
    };
    tView.tWorld.ForEachAlive([&](Handle, const Entity& tEntity) {
        if (tEntity.eType == EntityType::Enemy && tEntity.fHp > 0.0f)
            fnDot(tEntity.tBody, 2.0f, kHpColor);
    });
    if (const Entity* pPlayer = tView.tWorld.Get(tView.hPlayer))
        fnDot(pPlayer->tBody, 3.0f, kPlayerDotColor);

    const Vec2 vCam = tView.tCamera.GetTopLeft();
    DrawRectangleLinesEx(Rectangle{fX0 + vCam.fX * fPxToMap, fY0 + vCam.fY * fPxToMap, kViewSize.fX * fPxToMap,
                                   kViewSize.fY * fPxToMap},
                         1.0f, Fade(WHITE, 0.6f));
    return static_cast<int>(fY0 + fH) + 8;
}

// 보스 방: 화면 위 가운데 큰 HP 바 (TODO: Boss 타입 생기면 보스만. 지금은 방 안 적 HP 합)
void DrawBossBar(const World& tWorld)
{
    float fHp = 0.0f;
    tWorld.ForEachAlive([&](Handle, const Entity& tEntity) {
        if (tEntity.eType == EntityType::Enemy)
            fHp += std::max(0.0f, tEntity.fHp);
    });
    constexpr int kBarW = 520;
    DrawBar((kScreenWidth - kBarW) / 2, kTop + 40, kBarW, 14, fHp / kChaserDef.fMaxHp, kBossColor);
}

// 문 열림: 화면 오른쪽 가운데에서 깜빡이는 화살표
void DrawDoorArrow()
{
    const float fBlink = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 6.0f);
    const float fX = static_cast<float>(kScreenWidth - kMargin);
    const float fY = static_cast<float>(kFrameOffsetY + kViewHeight / 2);
    DrawTriangle(Vector2{fX - 28.0f, fY - 22.0f}, Vector2{fX - 28.0f, fY + 22.0f}, Vector2{fX, fY},
                 Fade(kArrowColor, 0.4f + 0.6f * fBlink));
}

void DrawCentered(const char* szText, int iY, int iFontSize, Color tColor)
{
    DrawText(szText, (kScreenWidth - MeasureText(szText, iFontSize)) / 2, iY, iFontSize, tColor);
}

// 타이틀·게임 오버·승리: 큰 단어 1개 + Enter 키캡
void DrawPhaseScreen(const RoomFlow& tRoomFlow)
{
    const GamePhase ePhase = tRoomFlow.GetPhase();
    DrawRectangle(0, 0, kScreenWidth, kScreenHeight, Fade(BLACK, ePhase == GamePhase::Title ? 0.75f : 0.6f));

    const int iMidY = kScreenHeight / 2;
    if (ePhase == GamePhase::Title)
    {
        DrawCentered("MAIN", iMidY - 150, 96, kTitleColor);
        // 조작법 (타이틀에서 한 번 보여 줌. 튜토리얼 방에도 키 안내 있음)
        constexpr const char* kControls[] = {
            "<- ->  move        Z  jump (hold = higher)        X  attack",
            "SHIFT  tap = dash (invincible)   /   hold = run (jump higher)",
            "F1  debug        ESC  quit",
        };
        int iY = iMidY - 20;
        for (const char* szLine : kControls)
        {
            DrawCentered(szLine, iY, 20, Fade(kTitleColor, 0.8f));
            iY += 28;
        }
    }
    else if (ePhase == GamePhase::GameOver)
        DrawCentered("GAME OVER", iMidY - 80, 80, kHpColor);
    else
    {
        DrawCentered("CLEAR", iMidY - 90, 88, kArrowColor);
        const int iSeconds = static_cast<int>(tRoomFlow.GetPlayTime());
        DrawCentered(TextFormat("%02d:%02d", iSeconds / 60, iSeconds % 60), iMidY + 4, 36, kTitleColor);
    }

    const float fBlink = 0.55f + 0.45f * std::sin(static_cast<float>(GetTime()) * 4.0f);
    const Rectangle tCap = {static_cast<float>(kScreenWidth / 2 - 60), static_cast<float>(iMidY + 70), 120.0f, 40.0f};
    DrawRectangleRoundedLinesEx(tCap, 0.3f, 4, 2.0f, Fade(kTitleColor, fBlink));
    DrawCentered("ENTER", iMidY + 80, 22, Fade(kTitleColor, fBlink));
}
} // namespace

void DrawHud(const FrameView& tView)
{
    const RoomFlow& tRoomFlow = tView.tRoomFlow;
    if (tRoomFlow.GetPhase() == GamePhase::Playing)
    {
        if (const Entity* pPlayer = tView.tWorld.Get(tView.hPlayer))
            DrawPlayerBars(*pPlayer);
        const int iBelowMap = DrawMinimap(tView);  // 오른쪽 위: 미니맵 -> 방 진행 점 -> 남은 적 칸
        DrawRoomProgress(tRoomFlow, iBelowMap);

        const int iTotal = tRoomFlow.GetRoomEnemyTotal();
        const int iLeft = CountEnemiesLeft(tView.tWorld);
        if (iTotal > 0 && !tRoomFlow.GetRoomDef().bBossRoom)
            DrawEnemyCounter(iLeft, iTotal, iBelowMap + 28);
        if (iLeft > 0 && iLeft * 4 <= iTotal)  // 처음의 1/4 이하로 남으면 위치 안내
            DrawOffscreenEnemyArrows(tView.tWorld, tView.tCamera.GetTopLeft());

        if (tRoomFlow.GetRoomDef().bBossRoom)
            DrawBossBar(tView.tWorld);
        else if (tRoomFlow.GetRoomState() == RoomState::Cleared)
            DrawDoorArrow();
        return;
    }
    DrawPhaseScreen(tRoomFlow);
}
