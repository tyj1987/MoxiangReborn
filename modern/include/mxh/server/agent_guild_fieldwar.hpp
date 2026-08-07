//
// 1:1 port of MP_GUILD_FIELDWARUserMsgParser / MP_GUILD_FIELDWARServerMsgParser
// from legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 4014-4090.
//
// Legacy semantics (preserved verbatim):
// USER side: DECLARE/SUGGESTEND -> CheckGuildMasterLogin; DECLARE_ACCEPT -> CheckGuildFieldWarMoney;
//            default -> TransToMapServerMsgParser.
// SERVER side: 7 NOTIFY_TOMAP -> Broadcast2MapServerExceptOne; DECLARE_NACK -> find user by dwData1;
//              ADDMONEY_TOMAP -> AddGuildFieldWarMoney; RESULT_TOALLUSER -> broadcast to all users;
//              default -> TransToClientMsgParser.

#pragma once

#include <cstddef>
#include <cstdint>

namespace mxh::server {

inline constexpr std::uint8_t guild_fieldwar_category = 57u;

inline constexpr std::uint8_t guild_fieldwar_nack = 0u;
inline constexpr std::uint8_t guild_fieldwar_wait = 1u;
inline constexpr std::uint8_t guild_fieldwar_declare = 2u;
inline constexpr std::uint8_t guild_fieldwar_declare_nack = 3u;
inline constexpr std::uint8_t guild_fieldwar_declare_accept = 4u;
inline constexpr std::uint8_t guild_fieldwar_declare_deny = 5u;
inline constexpr std::uint8_t guild_fieldwar_declare_deny_notify_tomap = 6u;
inline constexpr std::uint8_t guild_fieldwar_start = 7u;
inline constexpr std::uint8_t guild_fieldwar_start_notify_tomap = 8u;
inline constexpr std::uint8_t guild_fieldwar_proc = 9u;
inline constexpr std::uint8_t guild_fieldwar_end = 10u;
inline constexpr std::uint8_t guild_fieldwar_end_notify_tomap = 11u;
inline constexpr std::uint8_t guild_fieldwar_suggestend = 12u;
inline constexpr std::uint8_t guild_fieldwar_suggestend_notify_tomap = 13u;
inline constexpr std::uint8_t guild_fieldwar_suggestend_nack = 14u;
inline constexpr std::uint8_t guild_fieldwar_suggestend_accept = 15u;
inline constexpr std::uint8_t guild_fieldwar_suggestend_accept_notify_tomap = 16u;
inline constexpr std::uint8_t guild_fieldwar_suggestend_deny = 17u;
inline constexpr std::uint8_t guild_fieldwar_suggestend_deny_notify_tomap = 18u;
inline constexpr std::uint8_t guild_fieldwar_surrender = 19u;
inline constexpr std::uint8_t guild_fieldwar_surrender_nack = 20u;
inline constexpr std::uint8_t guild_fieldwar_surrender_notify_tomap = 21u;
inline constexpr std::uint8_t guild_fieldwar_leveldown = 22u;
inline constexpr std::uint8_t guild_fieldwar_record = 23u;
inline constexpr std::uint8_t guild_fieldwar_addmoney = 24u;
inline constexpr std::uint8_t guild_fieldwar_removemoney = 25u;
inline constexpr std::uint8_t guild_fieldwar_addmoney_tomap = 26u;
inline constexpr std::uint8_t guild_fieldwar_result_toalluser = 27u;

struct GuildFieldWarUserSlot final {
    std::uint32_t dw_connection_index = 0u;
    bool in_user_table = false;
};

struct GuildFieldWarDword2Payload final {
    std::uint32_t dw_object_id = 0u;
    std::uint32_t dw_data1 = 0u;
    std::uint32_t dw_data2 = 0u;
};

struct GuildFieldWarNameDwordPayload final {
    std::uint32_t dw_object_id = 0u;
    std::uint32_t dw_data = 0u;
};

struct GuildFieldWarName2Payload final {
    std::uint32_t dw_object_id = 0u;
    std::uint32_t dw_data1 = 0u;
    std::uint32_t dw_data2 = 0u;
};

struct GuildFieldWarUserRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t dw_object_id = 0u;
    std::uint32_t dw_data = 0u;
    std::uint32_t dw_data1 = 0u;
    std::uint32_t dw_data2 = 0u;
    bool user_found = true;
};

struct GuildFieldWarServerRequest final {
    std::uint8_t protocol = 0u;
    std::uint32_t dw_object_id = 0u;
    GuildFieldWarDword2Payload dword2{};
    GuildFieldWarName2Payload name2{};
    std::size_t user_count = 0u;
    const GuildFieldWarUserSlot* users = nullptr;
    bool target_user_found = false;
    std::uint32_t target_user_conn = 0u;
};

enum class GuildFieldWarUserActionKind : std::uint8_t {
    drop_no_user,
    check_guild_master_login,
    check_guild_field_war_money,
    forward_to_map_server,
    forward_to_map_server_no_user,
};

struct GuildFieldWarUserAction final {
    GuildFieldWarUserActionKind kind = GuildFieldWarUserActionKind::forward_to_map_server;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t dw_object_id = 0u;
    std::uint32_t dw_data = 0u;
    std::uint32_t dw_data1 = 0u;
    std::uint32_t dw_data2 = 0u;
    bool forward_payload = true;
};

enum class GuildFieldWarServerActionKind : std::uint8_t {
    drop_no_user,
    broadcast_to_map_servers_except_source,
    send_to_target_user_if_found,
    add_guild_field_war_money,
    broadcast_to_all_users,
    forward_to_originating_client,
    drop_unknown_protocol,
};

struct GuildFieldWarServerAction final {
    GuildFieldWarServerActionKind kind = GuildFieldWarServerActionKind::drop_unknown_protocol;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t dw_object_id = 0u;
    std::uint32_t dw_data1 = 0u;
    std::uint32_t dw_data2 = 0u;
    std::uint32_t target_user_conn = 0u;
    std::size_t broadcast_count = 0u;
    bool target_resolved = false;
    bool forward_payload = true;
};

inline constexpr bool is_guild_fieldwar_notify_tomap(std::uint8_t p) noexcept {
    return p == guild_fieldwar_declare_deny_notify_tomap ||
           p == guild_fieldwar_start_notify_tomap ||
           p == guild_fieldwar_end_notify_tomap ||
           p == guild_fieldwar_suggestend_notify_tomap ||
           p == guild_fieldwar_suggestend_accept_notify_tomap ||
           p == guild_fieldwar_suggestend_deny_notify_tomap ||
           p == guild_fieldwar_surrender_notify_tomap;
}

inline GuildFieldWarUserAction classify_guild_fieldwar_user(
    const GuildFieldWarUserRequest& r) noexcept {
    GuildFieldWarUserAction out;
    out.reply_protocol = r.protocol;
    out.dw_object_id = r.dw_object_id;
    out.dw_data = r.dw_data;
    out.dw_data1 = r.dw_data1;
    out.dw_data2 = r.dw_data2;
    if (!r.user_found) {
        out.kind = GuildFieldWarUserActionKind::forward_to_map_server_no_user;
        return out;
    }
    switch (r.protocol) {
        case guild_fieldwar_declare:
        case guild_fieldwar_suggestend:
            out.kind = GuildFieldWarUserActionKind::check_guild_master_login;
            return out;
        case guild_fieldwar_declare_accept:
            out.kind = GuildFieldWarUserActionKind::check_guild_field_war_money;
            return out;
        default:
            out.kind = GuildFieldWarUserActionKind::forward_to_map_server;
            return out;
    }
}

inline GuildFieldWarServerAction classify_guild_fieldwar_server(
    const GuildFieldWarServerRequest& r) noexcept {
    GuildFieldWarServerAction out;
    out.reply_protocol = r.protocol;
    out.dw_object_id = r.dw_object_id;
    out.dw_data1 = r.dword2.dw_data1;
    out.dw_data2 = r.dword2.dw_data2;
    if (is_guild_fieldwar_notify_tomap(r.protocol)) {
        out.kind = GuildFieldWarServerActionKind::broadcast_to_map_servers_except_source;
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
    switch (r.protocol) {
        case guild_fieldwar_declare_nack: {
            out.kind = GuildFieldWarServerActionKind::send_to_target_user_if_found;
            out.dw_data1 = r.dword2.dw_data1;
            out.dw_data2 = r.dword2.dw_data2;
            if (r.target_user_found) {
                out.target_user_conn = r.target_user_conn;
                out.target_resolved = true;
            } else {
                out.target_resolved = false;
            }
            return out;
        }
        case guild_fieldwar_addmoney_tomap: {
            out.kind = GuildFieldWarServerActionKind::add_guild_field_war_money;
            out.dw_data1 = r.dword2.dw_data1;
            out.dw_data2 = r.dword2.dw_data2;
            return out;
        }
        case guild_fieldwar_result_toalluser: {
            out.kind = GuildFieldWarServerActionKind::broadcast_to_all_users;
            out.dw_data1 = r.name2.dw_data1;
            out.dw_data2 = r.name2.dw_data2;
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
        case guild_fieldwar_nack:
        case guild_fieldwar_wait:
        case guild_fieldwar_declare:
        case guild_fieldwar_declare_accept:
        case guild_fieldwar_declare_deny:
        case guild_fieldwar_start:
        case guild_fieldwar_proc:
        case guild_fieldwar_end:
        case guild_fieldwar_suggestend:
        case guild_fieldwar_suggestend_nack:
        case guild_fieldwar_suggestend_accept:
        case guild_fieldwar_suggestend_deny:
        case guild_fieldwar_surrender:
        case guild_fieldwar_surrender_nack:
        case guild_fieldwar_leveldown:
        case guild_fieldwar_record:
        case guild_fieldwar_addmoney:
        case guild_fieldwar_removemoney: {
            out.kind = GuildFieldWarServerActionKind::forward_to_originating_client;
            return out;
        }
        default: {
            out.kind = GuildFieldWarServerActionKind::drop_unknown_protocol;
            out.broadcast_count = 0u;
            out.target_resolved = false;
            return out;
        }
    }
}

}  // namespace mxh::server
