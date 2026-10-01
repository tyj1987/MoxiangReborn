#pragma once
#include <cstdint>
#include <optional>
#include <span>

namespace mxh::unity {
struct PickupResponse {
    std::uint32_t drop_id;
    std::uint16_t item_id;
    std::uint16_t count;
};

// Modern MapServer sends the same eight-byte carrier for Ack and Nack.
// A rejection identifies the drop, but has no awarded item or quantity.
inline std::optional<PickupResponse> decode_pickup_response(
    std::span<const std::uint8_t> bytes, bool accepted) {
    if (bytes.size() != 8) return std::nullopt;
    const auto u16 = [&](std::size_t p) {
        return static_cast<std::uint16_t>(bytes[p] | (bytes[p + 1] << 8));
    };
    PickupResponse response{
        static_cast<std::uint32_t>(u16(0)) | (static_cast<std::uint32_t>(u16(2)) << 16),
        u16(4), u16(6)};
    if (response.drop_id == 0) return std::nullopt;
    if (accepted ? (response.item_id == 0 || response.count == 0)
                 : (response.item_id != 0 || response.count != 0)) return std::nullopt;
    return response;
}
}
