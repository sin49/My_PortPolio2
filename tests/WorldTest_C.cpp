// World 스펙 테스트 (AI 작성). 사용자가 구현한 World가 이 테스트를 통과하면 완료.
// 테스트가 설계(docs/design/world.md)와 다르다고 생각되면 고치지 말고 먼저 논의.
#include "core/Entity_U.h"
#include "core/Handle_U.h"
#include "core/World_U.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

#include <doctest/doctest.h>

namespace
{
Entity MakeEnemy(float fX)
{
    Entity tEntity;
    tEntity.eType = EntityType::Enemy;
    tEntity.tBody.vPos = {fX, 0.0f};
    tEntity.fHp = 10.0f;
    return tEntity;
}

// 용량을 꽉 채우고 핸들 목록 반환
std::vector<Handle> FillWorld(World& tWorld)
{
    std::vector<Handle> vecHandles;
    for (std::uint32_t i = 0; i < World::kCapacity; ++i)
        vecHandles.push_back(tWorld.Create(MakeEnemy(static_cast<float>(i))));
    return vecHandles;
}
} // namespace

TEST_CASE("World: 기본 Handle은 무효")
{
    const Handle hNone;
    CHECK(hNone.IsNull());

    World tWorld;
    CHECK(tWorld.Get(hNone) == nullptr);
    CHECK(tWorld.GetAliveCount() == 0);
}

TEST_CASE("World: Create 후 Get으로 값 읽기·쓰기")
{
    World tWorld;
    const Handle hEnemy = tWorld.Create(MakeEnemy(5.0f));
    CHECK_FALSE(hEnemy.IsNull());
    CHECK(tWorld.GetAliveCount() == 1);

    Entity* pEntity = tWorld.Get(hEnemy);
    REQUIRE(pEntity != nullptr);
    CHECK(pEntity->eType == EntityType::Enemy);
    CHECK(pEntity->tBody.vPos.fX == 5.0f);

    pEntity->fHp = 3.0f;  // 포인터로 수정하면 World 안의 값이 바뀜
    CHECK(tWorld.Get(hEnemy)->fHp == 3.0f);
}

TEST_CASE("World: const World에서도 Get 가능")
{
    World tWorld;
    const Handle hEnemy = tWorld.Create(MakeEnemy(1.0f));
    const World& tReadOnly = tWorld;
    const Entity* pEntity = tReadOnly.Get(hEnemy);
    REQUIRE(pEntity != nullptr);
    CHECK(pEntity->tBody.vPos.fX == 1.0f);
}

TEST_CASE("World: Destroy는 예약만, FlushRemovals 후 무효 (결정 7-A)")
{
    World tWorld;
    const Handle hEnemy = tWorld.Create(MakeEnemy(1.0f));

    tWorld.Destroy(hEnemy);
    CHECK(tWorld.Get(hEnemy) != nullptr);  // Flush 전까지는 유효
    CHECK(tWorld.GetAliveCount() == 1);

    tWorld.FlushRemovals();
    CHECK(tWorld.Get(hEnemy) == nullptr);
    CHECK(tWorld.GetAliveCount() == 0);
}

TEST_CASE("World: 칸이 재사용돼도 옛 핸들은 무효 (세대 번호)")
{
    World tWorld;
    const std::vector<Handle> vecHandles = FillWorld(tWorld);
    const Handle hOld = vecHandles[10];

    tWorld.Destroy(hOld);
    tWorld.FlushRemovals();

    const Handle hReused = tWorld.Create(MakeEnemy(999.0f));  // 빈 칸은 hOld 자리 하나뿐
    CHECK(hReused.uIndex == hOld.uIndex);
    CHECK(hReused.uGen != hOld.uGen);
    CHECK_FALSE(hReused == hOld);

    CHECK(tWorld.Get(hOld) == nullptr);  // 옛 핸들로는 새 엔티티에 접근 불가
    REQUIRE(tWorld.Get(hReused) != nullptr);
    CHECK(tWorld.Get(hReused)->tBody.vPos.fX == 999.0f);
}

TEST_CASE("World: 용량 초과 시 throw (결정 4)")
{
    World tWorld;
    FillWorld(tWorld);
    CHECK(tWorld.GetAliveCount() == static_cast<int>(World::kCapacity));
    CHECK_THROWS_AS(tWorld.Create(MakeEnemy(0.0f)), std::runtime_error);
    CHECK(tWorld.GetAliveCount() == static_cast<int>(World::kCapacity));
}

TEST_CASE("World: 중복·무효 Destroy는 무시 (결정 8)")
{
    World tWorld;
    const Handle hEnemy = tWorld.Create(MakeEnemy(1.0f));

    tWorld.Destroy(hEnemy);
    tWorld.Destroy(hEnemy);                                  // 중복
    tWorld.Destroy(Handle{});                                // 무효 핸들
    tWorld.Destroy(Handle{.uIndex = 999999, .uGen = 1});     // 범위 밖
    tWorld.FlushRemovals();
    CHECK(tWorld.GetAliveCount() == 0);

    tWorld.Destroy(hEnemy);  // 이미 해제된 핸들
    tWorld.FlushRemovals();

    // 빈 칸 목록에 같은 칸이 두 번 들어갔다면 256개를 넘게 만들 수 있게 됨
    FillWorld(tWorld);
    CHECK_THROWS_AS(tWorld.Create(MakeEnemy(0.0f)), std::runtime_error);
}

TEST_CASE("World: 범위 밖 index 핸들은 nullptr")
{
    World tWorld;
    tWorld.Create(MakeEnemy(1.0f));
    CHECK(tWorld.Get(Handle{.uIndex = World::kCapacity, .uGen = 1}) == nullptr);
    CHECK(tWorld.Get(Handle{.uIndex = 999999, .uGen = 1}) == nullptr);
}

TEST_CASE("World: ForEachAlive는 살아 있는 엔티티만 돎")
{
    World tWorld;
    std::vector<Handle> vecHandles;
    for (int i = 0; i < 5; ++i)
        vecHandles.push_back(tWorld.Create(MakeEnemy(static_cast<float>(i))));

    tWorld.Destroy(vecHandles[1]);
    int iVisited = 0;
    tWorld.ForEachAlive([&](Handle, Entity&) { ++iVisited; });
    CHECK(iVisited == 5);  // 예약만 된 상태는 아직 살아 있음 (결정 7-A)

    tWorld.FlushRemovals();
    iVisited = 0;
    tWorld.ForEachAlive([&](Handle hEntity, Entity& tEntity) {
        ++iVisited;
        CHECK(tWorld.Get(hEntity) == &tEntity);  // 넘겨준 핸들로 같은 엔티티를 가리킴
    });
    CHECK(iVisited == 4);
}

TEST_CASE("World: ForEachAlive 도중 Destroy해도 안전")
{
    World tWorld;
    for (int i = 0; i < 5; ++i)
        tWorld.Create(MakeEnemy(static_cast<float>(i)));

    int iVisited = 0;
    tWorld.ForEachAlive([&](Handle hEntity, Entity&) {
        ++iVisited;
        tWorld.Destroy(hEntity);  // 순회 중 삭제 -> 예약만 되므로 순회가 깨지지 않음
    });
    CHECK(iVisited == 5);
    CHECK(tWorld.GetAliveCount() == 5);

    tWorld.FlushRemovals();
    CHECK(tWorld.GetAliveCount() == 0);
    iVisited = 0;
    tWorld.ForEachAlive([&](Handle, Entity&) { ++iVisited; });
    CHECK(iVisited == 0);
}
