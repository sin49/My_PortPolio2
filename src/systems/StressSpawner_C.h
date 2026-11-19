#pragma once
// 스트레스 테스트용 적 소환 (docs/design/stress-sound.md 1장). Spatial Hash 측정 때 엔티티 수를 늘리는 용도.
// 방 진행 규칙은 모름 -> "클리어 안 된 방에서만"은 Game이 RoomFlow를 보고 판단.
#include "core/Handle_U.h"

#include <cstdint>
#include <random>

class Tilemap;
class World;

struct StressSpawnDef
{
    float fMinPlayerDistance = 220.0f;  // 플레이어 중심에서 이 거리 안에는 안 만듦 (완전 근처 제외)
    int iMaxTries = 40;                 // 랜덤 자리를 이만큼 뽑아도 빈 곳이 없으면 이번엔 포기
    // 남겨 둘 World 칸. 11/19 사용자 결정: 0 = 제한 없이 풀이 꽉 찰 때까지 (극한 테스트용)
    // 풀이 꽉 차도 안 죽음: 투사체·드랍 생성은 빈 칸이 없으면 건너뜀 (Projectiles::Spawn, Items::SpawnDrop)
    int iReservedSlots = 0;
};

inline constexpr StressSpawnDef kStressSpawnDef{};

class StressSpawner
{
public:
    explicit StressSpawner(std::uint32_t uSeed = 20261120u);

    // 작은 Chaser 1마리를 방 안 빈 곳(벽과 안 겹침, 플레이어 근처 제외) 랜덤 위치에. 만들었으면 true
    // false = World 풀이 꽉 참 또는 자리를 못 찾음
    bool SpawnSmallChaser(World& tWorld, const Tilemap& tRoom, Handle hPlayer,
                          const StressSpawnDef& tDef = kStressSpawnDef);

private:
    std::mt19937 m_tRng;
};
