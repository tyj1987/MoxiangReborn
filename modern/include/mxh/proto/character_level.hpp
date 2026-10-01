#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace mxh::proto {
// Packed MSG_LEVEL after MSGBASE: WORD Level, INT64 CurExpPoint, INT64 MaxExpPoint.
struct CharacterLevel {
    std::uint16_t level;
    std::int64_t current_experience;
    std::int64_t maximum_experience;
};
inline bool valid_character_level(const CharacterLevel& value) {
    // Current experience may exceed the next threshold: one transition per award.
    return value.level>0 && value.level<121 && value.current_experience>=0 &&
        value.maximum_experience>0;
}
inline std::optional<std::array<std::uint8_t,18>> encode_character_level(CharacterLevel value) {
    if(!valid_character_level(value)) return std::nullopt;
    std::array<std::uint8_t,18> bytes{};
    bytes[0]=static_cast<std::uint8_t>(value.level);
    bytes[1]=static_cast<std::uint8_t>(value.level>>8);
    for(unsigned i=0;i<8;++i) {
        bytes[2+i]=static_cast<std::uint8_t>(std::uint64_t(value.current_experience)>>(8*i));
        bytes[10+i]=static_cast<std::uint8_t>(std::uint64_t(value.maximum_experience)>>(8*i));
    }
    return bytes;
}
inline std::optional<CharacterLevel> decode_character_level(std::span<const std::uint8_t> bytes) {
    if(bytes.size()!=18 || (bytes[9]&128) || (bytes[17]&128)) return std::nullopt;
    std::uint64_t current=0,maximum=0;
    for(unsigned i=0;i<8;++i) {
        current|=std::uint64_t(bytes[2+i])<<(8*i);
        maximum|=std::uint64_t(bytes[10+i])<<(8*i);
    }
    CharacterLevel value{static_cast<std::uint16_t>(bytes[0]|(bytes[1]<<8)),
        static_cast<std::int64_t>(current),static_cast<std::int64_t>(maximum)};
    if(!valid_character_level(value)) return std::nullopt;
    return value;
}
}
