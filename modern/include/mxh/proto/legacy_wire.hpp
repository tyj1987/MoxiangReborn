#pragma once

#include <cstdint>
#include <algorithm>
#include <optional>
#include <span>
#include <vector>

namespace mxh::proto {

struct LegacyWireMessage {
    std::uint8_t checksum = 0;
    std::int8_t code = 0;
    std::uint8_t category = 0;
    std::uint8_t protocol = 0;
    std::uint32_t object_id = 0;
    std::vector<std::uint8_t> payload;
};

inline std::optional<std::vector<std::uint8_t>> encode_legacy_wire(const LegacyWireMessage& message) {
    if (message.payload.size() > 65527u) return std::nullopt;
    const auto bodySize = static_cast<std::uint16_t>(8u + message.payload.size());
    std::vector<std::uint8_t> bytes(2u + bodySize);
    bytes[0] = static_cast<std::uint8_t>(bodySize & 0xffu);
    bytes[1] = static_cast<std::uint8_t>((bodySize >> 8u) & 0xffu);
    bytes[2] = message.checksum;
    bytes[3] = static_cast<std::uint8_t>(message.code);
    bytes[4] = message.category;
    bytes[5] = message.protocol;
    bytes[6] = static_cast<std::uint8_t>(message.object_id & 0xffu);
    bytes[7] = static_cast<std::uint8_t>((message.object_id >> 8u) & 0xffu);
    bytes[8] = static_cast<std::uint8_t>((message.object_id >> 16u) & 0xffu);
    bytes[9] = static_cast<std::uint8_t>((message.object_id >> 24u) & 0xffu);
    std::copy(message.payload.begin(), message.payload.end(), bytes.begin() + 10);
    return bytes;
}

inline std::optional<LegacyWireMessage> decode_legacy_wire(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 10u) return std::nullopt;
    const auto bodySize = static_cast<std::uint16_t>(bytes[0]) |
                          (static_cast<std::uint16_t>(bytes[1]) << 8u);
    if (bodySize < 8u || bytes.size() != static_cast<std::size_t>(bodySize) + 2u)
        return std::nullopt;
    LegacyWireMessage message;
    message.checksum = bytes[2];
    message.code = static_cast<std::int8_t>(bytes[3]);
    message.category = bytes[4];
    message.protocol = bytes[5];
    message.object_id = static_cast<std::uint32_t>(bytes[6]) |
                        (static_cast<std::uint32_t>(bytes[7]) << 8u) |
                        (static_cast<std::uint32_t>(bytes[8]) << 16u) |
                        (static_cast<std::uint32_t>(bytes[9]) << 24u);
    message.payload.assign(bytes.begin() + 10, bytes.end());
    return message;
}

} // namespace mxh::proto
