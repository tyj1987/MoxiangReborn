#pragma once
#include <atomic>
#include <cstdint>
#include <optional>

namespace mxh::server {
// The high bit belongs to pending grant IDs (0x80000000 | grant_id). Ordinary
// pickups and purchases remain in the lower half and never wrap into grants.
inline constexpr std::uint32_t kFirstGrantItemId = 0x80000000u;

// Reserve from the handler's existing database-seeded item counter. A failed
// insertion may leave a gap; never return an ID to the counter or wrap to zero.
inline std::optional<std::uint32_t> reserve_item_id(std::atomic<std::uint32_t>& next) noexcept {
    auto candidate = next.load(std::memory_order_relaxed);
    while (candidate != 0 && candidate < kFirstGrantItemId) {
        if (next.compare_exchange_weak(candidate, candidate + 1, std::memory_order_relaxed))
            return candidate;
    }
    return std::nullopt;
}
}
