#pragma once

#include <cstdint>

// 엔티티 참조. World 칸 번호 + 세대 번호. uGen 0 = 무효 (살아 있는 칸은 항상 1 이상)
struct Handle
{
    std::uint32_t uIndex = 0;
    std::uint32_t uGen = 0;

    bool IsNull() const { return uGen == 0; }
    bool operator==(const Handle&) const = default;
};
