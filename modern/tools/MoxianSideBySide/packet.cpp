#include "packet.hpp"

namespace mxh::tools::sidebyside {

std::vector<std::uint8_t> Packet::wire_bytes() const {
    const auto bodyLength = static_cast<std::uint16_t>(8u + payload.size());
    std::vector<std::uint8_t> out(2u + bodyLength);
    out[0] = static_cast<std::uint8_t>(bodyLength & 0xffu);
    out[1] = static_cast<std::uint8_t>((bodyLength >> 8u) & 0xffu);
    out[2] = checksum;
    out[3] = static_cast<std::uint8_t>(code);
    out[4] = category;
    out[5] = protocol;
    out[6] = static_cast<std::uint8_t>(object_id & 0xffu);
    out[7] = static_cast<std::uint8_t>((object_id >> 8u) & 0xffu);
    out[8] = static_cast<std::uint8_t>((object_id >> 16u) & 0xffu);
    out[9] = static_cast<std::uint8_t>((object_id >> 24u) & 0xffu);
    std::copy(payload.begin(), payload.end(), out.begin() + 10);
    return out;
}

}  // namespace mxh::tools::sidebyside
