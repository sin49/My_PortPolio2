#pragma once

#include "core/Handle_U.h"

class Particles;
class Tilemap;
class World;

inline constexpr int kScreenWidth = 1280;
inline constexpr int kScreenHeight = 720;

// 루프 관련 디버그 값 (게임 상태가 아닌 것). Game이 채워서 넘김.
struct DebugInfo
{
    int iStepsThisFrame = 0;
    long long llTotalTicks = 0;
};

// 그리기만 함. 게임 상태는 const&로 읽기만.
// 방 좌표 -> 화면 좌표 변환(위·아래 프레임 오프셋)은 Render만 앎.
class Render
{
public:
    void Draw(const Tilemap& tRoom, const World& tWorld, Handle hPlayer, const Particles& tParticles,
              const DebugInfo& tDebug) const;
};
