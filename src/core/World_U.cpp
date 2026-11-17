#include "core/World_U.h"

#include <stdexcept>

World::World()
{
    m_vecSlots.resize(kCapacity);
    m_vecFreeList.reserve(kCapacity);
    m_vecPendingList.reserve(kCapacity);
    // 255, 254, ..., 0 순서로 넣음 -> 뒤에서 꺼내면 0번부터 사용
    for (int i = static_cast<int>(kCapacity) - 1; i >= 0; --i)
        m_vecFreeList.push_back(static_cast<std::uint32_t>(i));
}

Handle World::Create(const Entity& tInitial)
{
    if (m_vecFreeList.empty())
        throw std::runtime_error("World 용량 초과");

    const std::uint32_t uIndex = m_vecFreeList.back();
    m_vecFreeList.pop_back();

    Slot& tSlot = m_vecSlots[uIndex];
    tSlot.tValue = tInitial;
    tSlot.bAlive = true;
    tSlot.bPendingDestroy = false;
    return Handle{uIndex, tSlot.uGen};
}

Entity* World::Get(Handle hTarget)
{
    if (hTarget.uIndex >= kCapacity)
        return nullptr;
    Slot& tSlot = m_vecSlots[hTarget.uIndex];
    return (tSlot.bAlive && tSlot.uGen == hTarget.uGen) ? &tSlot.tValue : nullptr;
}

const Entity* World::Get(Handle hTarget) const
{
    if (hTarget.uIndex >= kCapacity)
        return nullptr;
    const Slot& tSlot = m_vecSlots[hTarget.uIndex];
    return (tSlot.bAlive && tSlot.uGen == hTarget.uGen) ? &tSlot.tValue : nullptr;
}

int World::GetAliveCount() const
{
    return static_cast<int>(kCapacity - m_vecFreeList.size());
}

void World::Destroy(Handle hTarget)
{
    if (Get(hTarget) == nullptr)
        return;  // 무효 핸들 무시
    Slot& tSlot = m_vecSlots[hTarget.uIndex];
    if (tSlot.bPendingDestroy)
        return;  // 중복 무시 (결정 8)
    tSlot.bPendingDestroy = true;
    m_vecPendingList.push_back(hTarget.uIndex);
}

void World::FlushRemovals()
{
    for (const std::uint32_t uIndex : m_vecPendingList)
    {
        Slot& tSlot = m_vecSlots[uIndex];
        tSlot.bAlive = false;
        tSlot.bPendingDestroy = false;
        tSlot.uGen += 1;  // 옛 핸들 전부 무효화
        tSlot.tValue = Entity{};
        m_vecFreeList.push_back(uIndex);
    }
    m_vecPendingList.clear();
}
