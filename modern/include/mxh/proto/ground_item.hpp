#pragma once

// Ground-item appearance payload. Distinct from the direct-to-inventory
// obtain array: a handler that holds one must not decode the other.
// 20 bytes: object id, source monster, item id, count, x, z.

#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <vector>

namespace mxh::proto {

inline constexpr std::size_t kGroundItemAddBytes = 20;

#pragma pack(push, 1)
struct GroundItemAddBody {
    std::uint32_t object_id = 0;
    std::uint32_t source_monster_id = 0;
    std::uint16_t item_id = 0;
    std::uint16_t count = 0;
    float pos_x = 0.0f;
    float pos_z = 0.0f;
};
#pragma pack(pop)
static_assert(sizeof(GroundItemAddBody) == kGroundItemAddBytes);

inline std::optional<std::vector<std::uint8_t>> encode_ground_item_add(
    const GroundItemAddBody& body) {
    if (body.object_id == 0u || body.item_id == 0u) return std::nullopt;
    std::vector<std::uint8_t> out(kGroundItemAddBytes);
    std::memcpy(out.data(), &body, kGroundItemAddBytes);
    return out;
}

inline std::optional<GroundItemAddBody> decode_ground_item_add(
    std::span<const std::uint8_t> bytes) {
    if (bytes.size() != kGroundItemAddBytes) return std::nullopt;
    GroundItemAddBody body{};
    std::memcpy(&body, bytes.data(), kGroundItemAddBytes);
    if (body.object_id == 0u || body.item_id == 0u) return std::nullopt;
    return body;
}

}  // namespace mxh::proto
