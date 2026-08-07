// agent_option.hpp - AgentOption data plane (category=36, MP_OPTION).
//
// 1:1 port of MP_OPTIONUserMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp lines 2707-2730.

#pragma once

#include <cstdint>

namespace mxh::server {

inline constexpr std::uint8_t option_category = 36u;

inline constexpr std::uint8_t option_set_syn = 0u;
inline constexpr std::uint8_t option_set_ack = 1u;
inline constexpr std::uint8_t option_set_nack = 2u;
inline constexpr std::uint8_t option_avatarview = 3u;

inline constexpr std::uint16_t legacy_opt_nowhisper = 0x0001u;
inline constexpr std::uint16_t legacy_opt_nofriend  = 0x0002u;

struct OptionUserRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    bool user_found = true;
    std::uint16_t option_bits = 0u;
};

enum class OptionUserActionKind : std::uint8_t {
    drop_no_user,
    forward_set_syn_to_map,
    forward_avatarview_to_map,
    forward_default,
};

struct OptionUserAction final {
    OptionUserActionKind kind = OptionUserActionKind::drop_no_user;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t object_id = 0u;
    std::uint16_t option_bits = 0u;
    bool no_whisper = false;
    bool no_friend = false;
    bool forward_to_map = false;
};

inline OptionUserAction classify_option_user(const OptionUserRequest& r) noexcept {
    OptionUserAction out;
    out.object_id = r.object_id;
    if (!r.user_found) {
        out.kind = OptionUserActionKind::drop_no_user;
        out.reply_protocol = r.protocol;
        return out;
    }
    switch (r.protocol) {
        case option_set_syn: {
            out.kind = OptionUserActionKind::forward_set_syn_to_map;
            out.reply_protocol = option_set_syn;
            out.option_bits = r.option_bits;
            out.no_whisper = (r.option_bits & legacy_opt_nowhisper) != 0u;
            out.no_friend  = (r.option_bits & legacy_opt_nofriend)  != 0u;
            out.forward_to_map = true;
            return out;
        }
        case option_avatarview:
            out.kind = OptionUserActionKind::forward_avatarview_to_map;
            out.reply_protocol = option_avatarview;
            out.forward_to_map = true;
            return out;
        default:
            out.kind = OptionUserActionKind::forward_default;
            out.reply_protocol = r.protocol;
            out.forward_to_map = true;
            return out;
    }
}

}  // namespace mxh::server
