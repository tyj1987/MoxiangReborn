#pragma once
#include <cstdint>
#include <optional>
#include <span>

namespace mxh::unity {
struct ShopListView {
    std::uint32_t npc_id;
    std::uint16_t count;
    std::span<const std::uint8_t> entries;
};

inline std::optional<ShopListView> parse_shop_list(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 6) return std::nullopt;
    std::uint32_t npc = 0;
    for (unsigned i = 0; i < 4; ++i) npc |= static_cast<std::uint32_t>(bytes[i]) << (8 * i);
    const auto count = static_cast<std::uint16_t>(bytes[4] | (bytes[5] << 8));
    if (bytes.size() != 6u + static_cast<std::size_t>(count) * 6u) return std::nullopt;
    // An unresolved dealer may legitimately return an empty catalog with ID 0.
    if (npc == 0 && count != 0) return std::nullopt;
    return ShopListView{npc, count, bytes.subspan(6)};
}
} // namespace mxh::unity
