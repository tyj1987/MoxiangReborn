#pragma once

#include "mxh/proto/protocol.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace mxh::server {

// This is the first, deliberately limited original-client revive capability.
// The caller must establish the map kind and session flags from authoritative
// state; false is not a substitute for an unknown value.
struct PresentReviveContext {
    std::uint32_t player_id = 0;
    std::uint16_t level = 0;
    std::uint16_t map_num = 0;
    bool ordinary_map_verified = false;
    bool dead = false;
    bool ready = false;
    bool looted = false;
    bool exiting = false;
    bool changing_map = false;
    bool sos = false;
    std::uint32_t max_hp = 0;
    std::uint32_t max_shield = 0;
    float pos_x = 0;
    float pos_z = 0;
};

struct PresentReviveCandidate {
    std::uint32_t player_id = 0;
    std::uint32_t hp = 0;
    std::uint32_t shield = 0;
    std::uint32_t mp = 0;
    std::uint16_t pos_x = 0;
    std::uint16_t pos_z = 0;
};

// GameResourceManager.cpp reads MapKindInfo.bin as tab-delimited rows:
// map number, map name, zero or more flags, '#'. Only a single unambiguous
// MAPVIEW-only row is admitted here. Unknown/special flags fail closed.
inline bool verified_ordinary_map_kind(std::span<const std::uint8_t> decoded,
                                       std::uint16_t map_num) noexcept {
    if (map_num == 0 || decoded.empty()) return false;
    const std::string_view text(reinterpret_cast<const char*>(decoded.data()), decoded.size());
    bool found = false;
    std::size_t line_start = 0;
    while (line_start < text.size()) {
        const auto line_end = text.find('\n', line_start);
        auto line = text.substr(line_start, line_end == std::string_view::npos
            ? text.size() - line_start : line_end - line_start);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        const auto first_tab = line.find('\t');
        if (first_tab != std::string_view::npos) {
            std::uint32_t row_map = 0;
            const auto number = line.substr(0, first_tab);
            const auto [end, error] = std::from_chars(number.data(),
                number.data() + number.size(), row_map);
            if (error == std::errc{} && end == number.data() + number.size() &&
                row_map == map_num) {
                if (found) return false;
                found = true;
                const auto name_end = line.find('\t', first_tab + 1);
                if (name_end == std::string_view::npos || name_end == first_tab + 1 ||
                    line.substr(name_end + 1) != "MAPVIEW\t#") return false;
            }
        }
        if (line_end == std::string_view::npos) break;
        line_start = line_end + 1;
    }
    return found;
}

inline std::optional<PresentReviveCandidate>
prepare_present_revive(const PresentReviveContext& c) noexcept {
    if (!c.player_id || c.level != 1 || !c.map_num ||
        !c.ordinary_map_verified || !c.dead || !c.ready || c.looted ||
        c.exiting || c.changing_map || c.sos || !c.max_hp ||
        !std::isfinite(c.pos_x) || !std::isfinite(c.pos_z) ||
        c.pos_x < 0 || c.pos_z < 0 || c.pos_x > 65535 || c.pos_z > 65535) {
        return std::nullopt;
    }
    // Original [Server]Map/Player.cpp:2335 casts max * 0.3 to DWORD.
    const auto hp = static_cast<std::uint32_t>(static_cast<double>(c.max_hp) * 0.3);
    const auto shield = static_cast<std::uint32_t>(static_cast<double>(c.max_shield) * 0.3);
    if (!hp) return std::nullopt;
    return PresentReviveCandidate{c.player_id, hp, shield, 0,
        static_cast<std::uint16_t>(c.pos_x), static_cast<std::uint16_t>(c.pos_z)};
}

inline bool is_present_spot_syn(std::uint8_t protocol,
                                std::span<const std::uint8_t> payload) noexcept {
    return protocol == 0 && payload.empty();
}

// Original CommonStruct.h MOVE_POS: DWORD mover + WORD x + WORD z.
inline std::array<std::uint8_t, 8>
encode_character_revive_move(const PresentReviveCandidate& c) noexcept {
    return {static_cast<std::uint8_t>(c.player_id),
            static_cast<std::uint8_t>(c.player_id >> 8),
            static_cast<std::uint8_t>(c.player_id >> 16),
            static_cast<std::uint8_t>(c.player_id >> 24),
            static_cast<std::uint8_t>(c.pos_x),
            static_cast<std::uint8_t>(c.pos_x >> 8),
            static_cast<std::uint8_t>(c.pos_z),
            static_cast<std::uint8_t>(c.pos_z >> 8)};
}

// Original MP_CHAR_NAERYUK_ACK is MSG_DWORD, an absolute value.
inline std::array<std::uint8_t, 4> encode_naeryuk_absolute(std::uint32_t mp) noexcept {
    return {static_cast<std::uint8_t>(mp), static_cast<std::uint8_t>(mp >> 8),
            static_cast<std::uint8_t>(mp >> 16), static_cast<std::uint8_t>(mp >> 24)};
}

static_assert(static_cast<std::uint8_t>(mxh::proto::Category::CharRevive) == 32);
static_assert(static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::CharacterRevive) == 44);

} // namespace mxh::server
