#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <span>

namespace mxh::proto {
// Original MOVE_POS after MSGBASE: DWORD dwMoverID, WORD wx, WORD wz.
// COMPRESSEDPOS drops Y and truncates valid nonnegative game coordinates.
struct CharacterRevivePosition {
    std::uint32_t player;
    std::uint16_t x,z;
};
inline std::optional<std::array<std::uint8_t,8>> encode_character_revive(
    std::uint32_t player,float x,float z) {
    if(!player || !std::isfinite(x) || !std::isfinite(z) ||
        x<0 || z<0 || x>=65536 || z>=65536) return std::nullopt;
    std::array<std::uint8_t,8> result{};
    for(unsigned i=0;i<4;++i) result[i]=static_cast<std::uint8_t>(player>>(i*8));
    const auto wx=static_cast<std::uint16_t>(x),wz=static_cast<std::uint16_t>(z);
    result[4]=static_cast<std::uint8_t>(wx); result[5]=static_cast<std::uint8_t>(wx>>8);
    result[6]=static_cast<std::uint8_t>(wz); result[7]=static_cast<std::uint8_t>(wz>>8);
    return result;
}
inline std::optional<CharacterRevivePosition> decode_character_revive(
    std::uint32_t header_player,std::span<const std::uint8_t> payload) {
    if(payload.size()!=8 || !header_player) return std::nullopt;
    const auto player=std::uint32_t(payload[0])|(std::uint32_t(payload[1])<<8)|
        (std::uint32_t(payload[2])<<16)|(std::uint32_t(payload[3])<<24);
    if(player!=header_player) return std::nullopt;
    return CharacterRevivePosition{player,
        static_cast<std::uint16_t>(payload[4]|(payload[5]<<8)),
        static_cast<std::uint16_t>(payload[6]|(payload[7]<<8))};
}
} // namespace mxh::proto
