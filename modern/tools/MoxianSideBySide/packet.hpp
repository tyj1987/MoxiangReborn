// packet.hpp - side-by-side packet representation.
#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace mxh::tools::sidebyside {

struct Packet {
    std::uint8_t  checksum = 0;
    std::int8_t   code     = 0;
    std::uint8_t  category = 0;
    std::uint8_t  protocol = 0;
    std::uint32_t object_id = 0;
    std::uint32_t length = 0;
    std::vector<std::uint8_t> payload;
    std::string   direction;
    std::uint64_t timestamp_ns = 0;

    std::vector<std::uint8_t> wire_bytes() const;
};

struct PacketDiff {
    std::size_t  index = 0;
    std::string  direction;
    std::uint8_t category_a = 0;
    std::uint8_t protocol_a = 0;
    std::uint8_t category_b = 0;
    std::uint8_t protocol_b = 0;
    std::size_t  first_diff_offset = 0;
    std::uint8_t expected_byte = 0;
    std::uint8_t actual_byte = 0;
    bool bytes_equal() const { return first_diff_offset == SIZE_MAX; }
};

}  // namespace mxh::tools::sidebyside
