#pragma once

#include "core/Entity_U.h"
#include "core/Handle_U.h"

#include <cstdint>
#include <vector>

// 엔티티 고정 슬롯 풀 (설계: docs/design/world.md)
// - 칸은 생성자에서 kCapacity개를 한 번 만들고 크기를 바꾸지 않음 -> Entity 주소가 이동하지 않음
// - Destroy는 예약만, FlushRemovals에서 실제 해제 + 세대 증가 (옛 핸들 무효화)
class World
{
public:
    static constexpr std::uint32_t kCapacity = 256;

    World();

    Handle Create(const Entity& tInitial);  // 빈 칸이 없으면 throw std::runtime_error
    Entity* Get(Handle hTarget);            // 무효·죽은·세대 불일치·범위 밖 -> nullptr
    const Entity* Get(Handle hTarget) const;
    void Destroy(Handle hTarget);           // 예약만. 무효·이미 예약된 핸들은 무시
    void FlushRemovals();
    int GetAliveCount() const;              // 살아 있는(예약 포함) 엔티티 수

    // fnVisit(Handle, Entity&). 순회 중 Destroy는 예약만 하므로 안전.
    template <typename Fn>
    void ForEachAlive(Fn&& fnVisit) 
    {
        for (std::uint32_t uIndex = 0; uIndex < kCapacity; ++uIndex)
        {
            Slot& tSlot = m_vecSlots[uIndex];
            if (tSlot.bAlive)
                fnVisit(Handle{uIndex, tSlot.uGen}, tSlot.tValue);
        }
    }
    template <typename Fn>
    void ForEachAlive(Fn&& fnVisit) const
    {
        for (std::uint32_t uIndex = 0; uIndex < kCapacity; ++uIndex)
        {
           const Slot& tSlot = m_vecSlots[uIndex];
            if (tSlot.bAlive)
                fnVisit(Handle{ uIndex, tSlot.uGen }, tSlot.tValue);
        }
    }

private:
    struct Slot
    {
        Entity tValue;
        std::uint32_t uGen = 1;
        bool bAlive = false;
        bool bPendingDestroy = false;
    };

    std::vector<Slot> m_vecSlots;
    std::vector<std::uint32_t> m_vecFreeList;     // 빈 칸 번호. 뒤에서 꺼냄
    std::vector<std::uint32_t> m_vecPendingList;  // 삭제 예약된 칸 번호
};
