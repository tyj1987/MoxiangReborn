//
// 1:1 port of MP_BOBUSANGUserMsgParser / MP_BOBUSANGServerMsgParser from
// legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 5191-5234.
//
// Legacy semantics (preserved verbatim):
// USER side (line 5191):
//   find user by dwObjectID; if not found -> return (drop).
//   if user is GM and GMPower > MASTER -> return (drop).
//   default -> TransToMapServerMsgParser.
// SERVER side (line 5212):
//   MP_BOBUSANG_APPEAR_MAP_TO_AGENT    -> BOBUSANGMGR->SetChannelState(dwData, APPEAR).
//   MP_BOBUSANG_DISAPPEAR_MAP_TO_AGENT -> BOBUSANGMGR->SetChannelState(dwData, DISAPPEAR).
//   default -> TransToClientMsgParser.
//
// This header is a pure data-plane classifier: it computes dispatch
// intent (kind, reply_protocol, channel, gm_overshoot) from the
// protocol + GM power + the channel payload, but does NOT touch the
// network sockets, BOBUSANGMGR singleton, or GM power table.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_BOBUSANG (74 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t bobusang_category = 74u;

// Sub-protocols within MP_PROTOCOL_BOBUSANG (see [CC]Header/Protocol.h
// enum at lines 3155-3169). Offsets are 0-based as in legacy source.
inline constexpr std::uint8_t bobusang_info_agent_to_map = 0u;
inline constexpr std::uint8_t bobusang_disappear_agent_to_map = 1u;
inline constexpr std::uint8_t bobusang_appear_map_to_agent = 2u;
inline constexpr std::uint8_t bobusang_disappear_map_to_agent = 3u;
inline constexpr std::uint8_t bobusang_add_guest_syn = 4u;
inline constexpr std::uint8_t bobusang_add_guest_ack = 5u;
inline constexpr std::uint8_t bobusang_add_guest_nack = 6u;
inline constexpr std::uint8_t bobusang_leave_guest_syn = 7u;
inline constexpr std::uint8_t bobusang_leave_guest_ack = 8u;
inline constexpr std::uint8_t bobusang_leave_guest_nack = 9u;
inline constexpr std::uint8_t bobusang_all_dealiteminfo_to_guest = 10u;
inline constexpr std::uint8_t bobusang_dealiteminfo_to_guest = 11u;
inline constexpr std::uint8_t bobusang_notify_for_disappearance = 12u;

// Legacy GM power sentinel (>= GM_POWER_MASTER blocks the user-side
// forwarder; legacy uses > eGM_POWER_MASTER so ==MASTER still forwards).
inline constexpr std::uint8_t bobusang_gm_power_master = 5u;
inline constexpr std::uint8_t bobusang_gm_power_max = 9u;
inline constexpr std::uint8_t bobusang_user_level_gm = 9u;

// Channel state enum preserved 1:1 from the legacy BOBUSANGMGR.
enum class BobusangChannelState : std::uint8_t {
    Appear = 0u,
    Disappear = 1u,
};

// Payload for legacy MSG_DWORD (used by APPEAR/DISAPPEAR_MAP_TO_AGENT).
struct BobusangDwordPayload final {
    std::uint32_t dw_object_id = 0u;
    std::uint32_t dw_data = 0u;  // channel id
};

// Input contract for classify_bobusang_user (client -> agent side).
struct BobusangUserRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t dw_object_id = 0u;
    bool user_found = true;
    bool user_is_gm = false;
    std::uint8_t gm_power = 0u;
};

// Input contract for classify_bobusang_server (other-server -> agent).
struct BobusangServerRequest final {
    std::uint8_t protocol = 0u;
    BobusangDwordPayload dword{};
};

// Dispatch intent for the user-side handler.
enum class BobusangUserActionKind : std::uint8_t {
    drop_no_user,
    drop_gm_overshoot,
    forward_to_map_server,
};

struct BobusangUserAction final {
    BobusangUserActionKind kind = BobusangUserActionKind::drop_no_user;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t dw_object_id = 0u;
    bool forward_payload = true;
};

// Dispatch intent for the server-side handler.
enum class BobusangServerActionKind : std::uint8_t {
    set_channel_state,
    forward_to_originating_client,
    drop_unknown_protocol,
};

struct BobusangServerAction final {
    BobusangServerActionKind kind = BobusangServerActionKind::drop_unknown_protocol;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t dw_object_id = 0u;
    std::uint32_t channel_id = 0u;
    BobusangChannelState state = BobusangChannelState::Appear;
    bool forward_payload = true;
};

inline BobusangUserAction classify_bobusang_user(
    const BobusangUserRequest& r) noexcept {
    BobusangUserAction out;
    out.reply_protocol = r.protocol;
    out.dw_object_id = r.dw_object_id;
    if (!r.user_found) {
        out.kind = BobusangUserActionKind::drop_no_user;
        return out;
    }
    if (r.user_is_gm && r.gm_power > bobusang_gm_power_master) {
        out.kind = BobusangUserActionKind::drop_gm_overshoot;
        return out;
    }
    out.kind = BobusangUserActionKind::forward_to_map_server;
    return out;
}

inline BobusangServerAction classify_bobusang_server(
    const BobusangServerRequest& r) noexcept {
    BobusangServerAction out;
    out.reply_protocol = r.protocol;
    out.dw_object_id = r.dword.dw_object_id;
    switch (r.protocol) {
        case bobusang_appear_map_to_agent: {
            out.kind = BobusangServerActionKind::set_channel_state;
            out.channel_id = r.dword.dw_data;
            out.state = BobusangChannelState::Appear;
            return out;
        }
        case bobusang_disappear_map_to_agent: {
            out.kind = BobusangServerActionKind::set_channel_state;
            out.channel_id = r.dword.dw_data;
            out.state = BobusangChannelState::Disappear;
            return out;
        }
        case bobusang_info_agent_to_map:
        case bobusang_disappear_agent_to_map:
        case bobusang_add_guest_syn:
        case bobusang_add_guest_ack:
        case bobusang_add_guest_nack:
        case bobusang_leave_guest_syn:
        case bobusang_leave_guest_ack:
        case bobusang_leave_guest_nack:
        case bobusang_all_dealiteminfo_to_guest:
        case bobusang_dealiteminfo_to_guest:
        case bobusang_notify_for_disappearance: {
            out.kind = BobusangServerActionKind::forward_to_originating_client;
            return out;
        }
        default: {
            out.kind = BobusangServerActionKind::drop_unknown_protocol;
            out.channel_id = 0u;
            return out;
        }
    }
}

}  // namespace mxh::server
