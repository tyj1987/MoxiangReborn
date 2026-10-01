#pragma once
#include <atomic>
#include <cstdint>
#include <limits>
#include <optional>

namespace mxh::server {
// Reserve from the handler's existing database-seeded item counter. A failed
// insertion may leave a gap; never return an ID to the counter or wrap to zero.
inline std::optional<std::uint32_t> reserve_item_id(std::atomic<std::uint32_t>& next) noexcept {
    auto candidate = next.load(std::memory_order_relaxed);
    while (candidate != 0 && candidate < std::numeric_limits<std::uint32_t>::max()) {
        if (next.compare_exchange_weak(candidate, candidate + 1, std::memory_order_relaxed))
            return candidate;
    }
    return std::nullopt;
}
}
