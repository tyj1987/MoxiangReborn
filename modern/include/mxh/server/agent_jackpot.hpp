//
// 1:1 port of MP_JACKPOTUserMsgParser / MP_JACKPOTServerMsgParser from
// legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 4614-4673.
//
// Legacy semantics (preserved verbatim):
//   - User side handler is empty: every MP_JACKPOT_* originating from a
//     client is silently dropped (no log, no response, no map forwarding).
//   - Server side handler dispatches by sub-protocol:
//       MP_JACKPOT_PRIZE_NOTIFY (0):
//           JACKPOTMGR->SetTotalMoney(pmsg->dwRestTotalMoney);
//           broadcast pmsg verbatim to every USERINFO in g_pUserTable
//           (no map filter; both in-map and lobby users receive it).
//       MP_JACKPOT_TOTALMONEY_NOTIFY (1):
//           legacy source block is commented out; falls through to the
//           default which forwards the packet to the originating client
//           via TransToClientMsgParser.
//       MP_JACKPOT_TOTALMONEY_NOTIFY_TO_AGENT (2):
//           JACKPOTMGR->SetTotalMoney(pmsg->dwData);
//           mutate pmsg->Protocol = MP_JACKPOT_TOTALMONEY_NOTIFY;
//           broadcast pmsg (rewritten protocol) to every USERINFO whose
//           wUserMapNum != 0 (i.e. users currently in a map server).
//       MP_JACKPOT_PRIZE_EFFECT (3):
//           default -> forward pmsg verbatim to originating client.
//       MP_JACKPOT_CHEAT_MAPTOTALMONEY (4):
//           default -> forward pmsg verbatim to originating client.
//
// This header is a pure data-plane classifier: it computes the dispatch
// intent (kind, reply_protocol, broadcast_targets, jackpot_total_money)
// from the protocol + the agent's view of connected users, but does NOT
// touch network sockets, the JACKPOTMGR singleton, or g_pUserTable.

#pragma once

#include <cstddef>
#include <cstdint>

namespace mxh::server {

inline constexpr std::uint8_t jackpot_category = 61u;

inline constexpr std::uint8_t jackpot_prize_notify = 0u;
inline constexpr std::uint8_t jackpot_totalmoney_notify = 1u;
inline constexpr std::uint8_t jackpot_totalmoney_notify_to_agent = 2u;
inline constexpr std::uint8_t jackpot_prize_effect = 3u;
inline constexpr std::uint8_t jackpot_cheat_maptotalmoney = 4u;

struct JackpotPrizeNotifyPayload final {
    std::uint32_t dw_object_id = 0u;
    std::uint32_t dw_rest_total_money = 0u;
};

struct JackpotTotalMoneyPayload final {
    std::uint32_t dw_object_id = 0u;
    std::uint32_t dw_data = 0u;
};

struct JackpotUserSlot final {
    std::uint32_t dw_connection_index = 0u;
    std::uint16_t w_user_map_num = 0u;
    bool in_user_table = false;
};

struct JackpotUserRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t dw_object_id = 0u;
    bool user_found = true;
};

struct JackpotServerRequest final {
    std::uint8_t protocol = 0u;
    JackpotPrizeNotifyPayload prize{};
    JackpotTotalMoneyPayload   total{};
    std::size_t user_count = 0u;
    const JackpotUserSlot* users = nullptr;
};

enum class JackpotUserActionKind : std::uint8_t {
    drop_no_user,
    drop_unknown_protocol,
    drop_no_handler,
};

struct JackpotUserAction final {
    JackpotUserActionKind kind = JackpotUserActionKind::drop_no_handler;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t dw_object_id = 0u;
};

enum class JackpotServerActionKind : std::uint8_t {
    broadcast_all_users,
    broadcast_in_map_users,
    forward_to_originating_client,
    drop_unknown_protocol,
};

struct JackpotServerAction final {
    JackpotServerActionKind kind = JackpotServerActionKind::drop_unknown_protocol;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t dw_object_id = 0u;
    std::uint32_t jackpot_total_money = 0u;
    bool rewrite_protocol = false;
    std::uint8_t rewritten_protocol = 0u;
    std::size_t broadcast_count = 0u;
};

inline JackpotUserAction classify_jackpot_user(
    const JackpotUserRequest& r) noexcept {
    JackpotUserAction out;
    out.reply_protocol = r.protocol;
    out.dw_object_id = r.dw_object_id;
    if (!r.user_found) {
        out.kind = JackpotUserActionKind::drop_no_user;
        return out;
    }
    out.kind = JackpotUserActionKind::drop_no_handler;
    return out;
}

inline JackpotServerAction classify_jackpot_server(
    const JackpotServerRequest& r) noexcept {
    JackpotServerAction out;
    switch (r.protocol) {
        case jackpot_prize_notify: {
            out.kind = JackpotServerActionKind::broadcast_all_users;
            out.reply_protocol = jackpot_prize_notify;
            out.dw_object_id = r.prize.dw_object_id;
            out.jackpot_total_money = r.prize.dw_rest_total_money;
            out.rewrite_protocol = false;
            out.rewritten_protocol = 0u;
            std::size_t count = 0u;
            if (r.users != nullptr) {
                for (std::size_t i = 0; i < r.user_count; ++i) {
                    if (r.users[i].in_user_table) {
                        ++count;
                    }
                }
            }
            out.broadcast_count = count;
            return out;
        }
        case jackpot_totalmoney_notify_to_agent: {
            out.kind = JackpotServerActionKind::broadcast_in_map_users;
            out.reply_protocol = jackpot_totalmoney_notify;
            out.dw_object_id = r.total.dw_object_id;
            out.jackpot_total_money = r.total.dw_data;
            out.rewrite_protocol = true;
            out.rewritten_protocol = jackpot_totalmoney_notify;
            std::size_t count = 0u;
            if (r.users != nullptr) {
                for (std::size_t i = 0; i < r.user_count; ++i) {
                    if (r.users[i].in_user_table &&
                        r.users[i].w_user_map_num != 0u) {
                        ++count;
                    }
                }
            }
            out.broadcast_count = count;
            return out;
        }
        case jackpot_totalmoney_notify:
        case jackpot_prize_effect:
        case jackpot_cheat_maptotalmoney: {
            out.kind = JackpotServerActionKind::forward_to_originating_client;
            out.reply_protocol = r.protocol;
            out.jackpot_total_money = 0u;
            out.rewrite_protocol = false;
            out.rewritten_protocol = 0u;
            out.broadcast_count = 0u;
            return out;
        }
        default: {
            out.kind = JackpotServerActionKind::drop_unknown_protocol;
            out.reply_protocol = r.protocol;
            out.jackpot_total_money = 0u;
            out.rewrite_protocol = false;
            out.rewritten_protocol = 0u;
            out.broadcast_count = 0u;
            return out;
        }
    }
}

}  // namespace mxh::server
