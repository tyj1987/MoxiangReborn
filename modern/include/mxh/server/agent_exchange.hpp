//
// 1:1 port of MP_EXCHANGEUserMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp lines 5095-5104.
//
// Legacy semantics (preserved verbatim):
//   - find user by dwConnectionIndex; if missing -> drop silently.
//   - if pUserInfo->dwCharacterID != pmsg->dwObjectID -> drop silently.
//   - default -> TransToMapServerMsgParser.
//
// The handler is user-side only; there is no MP_EXCHANGEServerMsgParser
// in the agent (exchange state changes flow map-server-side, never via
// the agent category dispatch). The data plane is intentionally a near-
// clone of MP_STREETSTALLUserMsgParser; both are simple user-found + 
// objectid integrity gates.

#pragma once

#include <cstdint>

namespace mxh::server {

// MP_CATEGORY byte for MP_EXCHANGE (28 in [CC]Header/Protocol.h).
inline constexpr std::uint8_t exchange_category = 28u;

// Sub-protocols within MP_PROTOCOL_EXCHANGE (see [CC]Header/Protocol.h
// enum at lines 1871-1932). Offsets are 0-based as in legacy source.
inline constexpr std::uint8_t exchange_apply = 0u;
inline constexpr std::uint8_t exchange_apply_syn = 1u;
inline constexpr std::uint8_t exchange_apply_ack = 2u;
inline constexpr std::uint8_t exchange_apply_nack = 3u;
inline constexpr std::uint8_t exchange_accept = 5u;
inline constexpr std::uint8_t exchange_accept_syn = 6u;
inline constexpr std::uint8_t exchange_accept_ack = 7u;
inline constexpr std::uint8_t exchange_accept_nack = 8u;
inline constexpr std::uint8_t exchange_cantapply = 10u;
inline constexpr std::uint8_t exchange_reject = 11u;
inline constexpr std::uint8_t exchange_reject_syn = 12u;
inline constexpr std::uint8_t exchange_waiting_cancel = 15u;
inline constexpr std::uint8_t exchange_waiting_cancel_syn = 16u;
inline constexpr std::uint8_t exchange_waiting_cancel_ack = 17u;
inline constexpr std::uint8_t exchange_waiting_cancel_nack = 18u;
inline constexpr std::uint8_t exchange_start = 20u;
inline constexpr std::uint8_t exchange_additem = 21u;
inline constexpr std::uint8_t exchange_additem_syn = 22u;
inline constexpr std::uint8_t exchange_additem_ack = 23u;
inline constexpr std::uint8_t exchange_additem_nack = 24u;
inline constexpr std::uint8_t exchange_delitem = 26u;
inline constexpr std::uint8_t exchange_delitem_syn = 27u;
inline constexpr std::uint8_t exchange_delitem_ack = 28u;
inline constexpr std::uint8_t exchange_delitem_nack = 29u;
inline constexpr std::uint8_t exchange_inputmoney = 31u;
inline constexpr std::uint8_t exchange_inputmoney_syn = 32u;
inline constexpr std::uint8_t exchange_inputmoney_ack = 33u;
inline constexpr std::uint8_t exchange_inputmoney_nack = 34u;
inline constexpr std::uint8_t exchange_lock = 36u;
inline constexpr std::uint8_t exchange_lock_syn = 37u;
inline constexpr std::uint8_t exchange_lock_ack = 38u;
inline constexpr std::uint8_t exchange_lock_nack = 39u;
inline constexpr std::uint8_t exchange_unlock = 41u;
inline constexpr std::uint8_t exchange_unlock_syn = 42u;
inline constexpr std::uint8_t exchange_unlock_ack = 43u;
inline constexpr std::uint8_t exchange_unlock_nack = 44u;
inline constexpr std::uint8_t exchange_exchange = 46u;
inline constexpr std::uint8_t exchange_exchange_syn = 47u;
inline constexpr std::uint8_t exchange_exchange_ack = 48u;
inline constexpr std::uint8_t exchange_exchange_nack = 49u;
inline constexpr std::uint8_t exchange_cancel = 51u;
inline constexpr std::uint8_t exchange_cancel_syn = 52u;
inline constexpr std::uint8_t exchange_cancel_ack = 53u;
inline constexpr std::uint8_t exchange_cancel_nack = 54u;
inline constexpr std::uint8_t exchange_insert = 56u;
inline constexpr std::uint8_t exchange_remove = 57u;
inline constexpr std::uint8_t exchange_setmoney = 58u;

// Input contract for classify_exchange_user (client -> agent side).
struct ExchangeUserRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t dw_object_id = 0u;
    std::uint32_t dw_user_character_id = 0u;
    bool user_found = true;
};

// Dispatch intent for the user-side handler.
enum class ExchangeUserActionKind : std::uint8_t {
    drop_no_user,
    drop_object_id_mismatch,
    forward_to_map_server,
};

struct ExchangeUserAction final {
    ExchangeUserActionKind kind = ExchangeUserActionKind::drop_no_user;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t dw_object_id = 0u;
    bool forward_payload = false;
};

inline ExchangeUserAction classify_exchange_user(
    const ExchangeUserRequest& r) noexcept {
    ExchangeUserAction out;
    out.reply_protocol = r.protocol;
    out.dw_object_id = r.dw_object_id;
    if (!r.user_found) {
        out.kind = ExchangeUserActionKind::drop_no_user;
        out.forward_payload = false;
        return out;
    }
    if (r.dw_user_character_id != r.dw_object_id) {
        out.kind = ExchangeUserActionKind::drop_object_id_mismatch;
        out.forward_payload = false;
        return out;
    }
    out.kind = ExchangeUserActionKind::forward_to_map_server;
    out.forward_payload = true;
    return out;
}

}  // namespace mxh::server
