//
// 1:1 port of MP_STREETSTALLUserMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp lines 5083-5092.
//
// Legacy semantics (preserved verbatim):
//   - find user by dwConnectionIndex; if missing -> drop silently.
//   - if pUserInfo->dwCharacterID != pmsg->dwObjectID -> drop silently.
//   - default -> TransToMapServerMsgParser.
//
// The handler is user-side only; there is no MP_STREETSTALLServerMsgParser
// in the agent. Server-originated streetstall traffic reaches map servers
// via direct distribution, never through the agent category dispatch.
//
// This header is a pure data-plane classifier: it computes dispatch
// intent (kind, reply_protocol, forward_payload) from the protocol + the
// user-by-conn lookup + the dwObjectID integrity check.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_STREETSTALL (29 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t streetstall_category = 29u;

// Sub-protocols within MP_PROTOCOL_STREETSTALL (see [CC]Header/Protocol.h
// enum at lines 1937-2018). All offsets preserved verbatim from legacy.
inline constexpr std::uint8_t streetstall_start = 0u;
inline constexpr std::uint8_t streetstall_end = 1u;
inline constexpr std::uint8_t streetstall_open_syn = 3u;
inline constexpr std::uint8_t streetstall_open_ack = 4u;
inline constexpr std::uint8_t streetstall_open_nack = 5u;
inline constexpr std::uint8_t streetstall_close_syn = 7u;
inline constexpr std::uint8_t streetstall_close_ack = 8u;
inline constexpr std::uint8_t streetstall_close_nack = 9u;
inline constexpr std::uint8_t streetstall_close = 10u;
inline constexpr std::uint8_t streetstall_lockitem_syn = 12u;
inline constexpr std::uint8_t streetstall_lockitem_ack = 13u;
inline constexpr std::uint8_t streetstall_lockitem_nack = 14u;
inline constexpr std::uint8_t streetstall_lockitem = 15u;
inline constexpr std::uint8_t streetstall_unlockitem_syn = 17u;
inline constexpr std::uint8_t streetstall_unlockitem_ack = 18u;
inline constexpr std::uint8_t streetstall_unlockitem_nack = 19u;
inline constexpr std::uint8_t streetstall_unlockitem = 20u;
inline constexpr std::uint8_t streetstall_deleteitem_syn = 22u;
inline constexpr std::uint8_t streetstall_deleteitem_ack = 23u;
inline constexpr std::uint8_t streetstall_deleteitem_nack = 24u;
inline constexpr std::uint8_t streetstall_deleteitem = 25u;
inline constexpr std::uint8_t streetstall_buyitem = 27u;
inline constexpr std::uint8_t streetstall_buyitem_error = 28u;
inline constexpr std::uint8_t streetstall_buyitem_syn = 29u;
inline constexpr std::uint8_t streetstall_buyitem_ack = 30u;
inline constexpr std::uint8_t streetstall_buyitem_nack = 31u;
inline constexpr std::uint8_t streetstall_sellitem = 32u;
inline constexpr std::uint8_t streetstall_sellitem_error = 33u;
inline constexpr std::uint8_t streetstall_sellitem_syn = 34u;
inline constexpr std::uint8_t streetstall_sellitem_ack = 35u;
inline constexpr std::uint8_t streetstall_sellitem_nack = 36u;
inline constexpr std::uint8_t streetstall_guestin_syn = 39u;
inline constexpr std::uint8_t streetstall_guestin_ack = 40u;
inline constexpr std::uint8_t streetstall_guestin_nack = 41u;
inline constexpr std::uint8_t streetstall_guestout_syn = 43u;
inline constexpr std::uint8_t streetstall_guestout_ack = 44u;
inline constexpr std::uint8_t streetstall_guestout_nack = 45u;
inline constexpr std::uint8_t streetstall_edittitle_syn = 47u;
inline constexpr std::uint8_t streetstall_edittitle_ack = 48u;
inline constexpr std::uint8_t streetstall_edittitle_nack = 49u;
inline constexpr std::uint8_t streetstall_edittitle = 50u;
inline constexpr std::uint8_t streetstall_updateitem = 52u;
inline constexpr std::uint8_t streetstall_update_syn = 53u;
inline constexpr std::uint8_t streetstall_update_ack = 54u;
inline constexpr std::uint8_t streetstall_update_nack = 55u;
inline constexpr std::uint8_t streetstall_update = 56u;
inline constexpr std::uint8_t streetstall_updateend_syn = 58u;
inline constexpr std::uint8_t streetstall_updateend_ack = 59u;
inline constexpr std::uint8_t streetstall_updateend_nack = 60u;
inline constexpr std::uint8_t streetstall_updateend = 61u;
inline constexpr std::uint8_t streetstall_fakeregistitem_syn = 63u;
inline constexpr std::uint8_t streetstall_fakeregistitem_ack = 64u;
inline constexpr std::uint8_t streetstall_fakeregistitem_nack = 65u;
inline constexpr std::uint8_t streetstall_fakeregistitem = 66u;
inline constexpr std::uint8_t streetstall_fakeregistbuyitem_syn = 68u;
inline constexpr std::uint8_t streetstall_fakeregistbuyitem_ack = 69u;
inline constexpr std::uint8_t streetstall_fakeregistbuyitem_nack = 70u;
inline constexpr std::uint8_t streetstall_fakeregistbuyitem = 71u;
inline constexpr std::uint8_t streetstall_message = 73u;
inline constexpr std::uint8_t streetstall_finditem_syn = 76u;
inline constexpr std::uint8_t streetstall_finditem_ack = 77u;
inline constexpr std::uint8_t streetstall_finditem_nack = 78u;
inline constexpr std::uint8_t streetstall_itemview_syn = 80u;
inline constexpr std::uint8_t streetstall_itemview_ack = 81u;
inline constexpr std::uint8_t streetstall_itemview_nack = 82u;

// Input contract for classify_streetstall_user (client -> agent side).
struct StreetStallUserRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t dw_object_id = 0u;
    std::uint32_t dw_user_character_id = 0u;
    bool user_found = true;
};

// Dispatch intent for the user-side handler.
enum class StreetStallUserActionKind : std::uint8_t {
    drop_no_user,
    drop_object_id_mismatch,
    forward_to_map_server,
};

struct StreetStallUserAction final {
    StreetStallUserActionKind kind = StreetStallUserActionKind::drop_no_user;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t dw_object_id = 0u;
    bool forward_payload = false;
};

inline StreetStallUserAction classify_streetstall_user(
    const StreetStallUserRequest& r) noexcept {
    StreetStallUserAction out;
    out.reply_protocol = r.protocol;
    out.dw_object_id = r.dw_object_id;
    if (!r.user_found) {
        out.kind = StreetStallUserActionKind::drop_no_user;
        out.forward_payload = false;
        return out;
    }
    if (r.dw_user_character_id != r.dw_object_id) {
        out.kind = StreetStallUserActionKind::drop_object_id_mismatch;
        out.forward_payload = false;
        return out;
    }
    out.kind = StreetStallUserActionKind::forward_to_map_server;
    out.forward_payload = true;
    return out;
}

}  // namespace mxh::server
