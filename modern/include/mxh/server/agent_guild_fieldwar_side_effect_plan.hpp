// D4.169 -- AgentGuildFieldWar side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_GUILD_FIELDWARUserMsgParser (lines 4014-4041) and MP_GUILD_FIELDWARServerMsgParser
// (lines 4043-4090). The data plane (classify_guild_fieldwar_user /
// classify_guild_fieldwar_server) decides the action; this header captures the ordered
// side-effect list the orchestrator must execute.
//
// Legacy semantics (preserved verbatim):
//   USER side:
//     DECLARE/SUGGESTEND -> CheckGuildMasterLogin.
//     DECLARE_ACCEPT     -> CheckGuildFieldWarMoney.
//     default            -> TransToMapServerMsgParser.
//     user not found     -> TransToMapServerMsgParser (no_user variant).
//   SERVER side:
//     *_NOTIFY_TOMAP   -> Broadcast2MapServerExceptOne (sender excluded).
//     DECLARE_NACK     -> find user by dwData1; Send2User if found else drop.
//     ADDMONEY_TOMAP   -> AddGuildFieldWarMoney(dwData1, dwData2).
//     RESULT_TOALLUSER -> broadcast to every USERINFO in g_pUserTable.
//     default          -> TransToClientMsgParser.
//     unknown          -> silent no-op.
//
// Side effects:
//   - Drop: silent no-op.
//   - ForwardToMapServer: route the original packet to map (with payload).
//   - ForwardToMapServerNoUser: same as ForwardToMapServer but user-not-found.
//   - CheckGuildMasterLogin: GUILDMGR->CheckGuildMasterLogin; on success, forward.
//   - CheckGuildFieldWarMoney: GUILDWARMGR->CheckMoney; on success, forward.
//   - BroadcastToMapServersExceptSource: BOBUSANGMGR-style; excludes sender map.
//   - SendToTargetUserIfFound: Send2User(target_user_conn) when target_resolved.
//   - AddGuildFieldWarMoney: GUILDWARMGR->AddMoney(dw_data1, dw_data2).
//   - BroadcastToAllUsers: Send2User for every USERINFO slot.
//   - ForwardToOriginatingClient: route the original packet back to the user.

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "mxh/server/agent_guild_fieldwar.hpp"

namespace mxh::server {

// Side-effect kinds the AgentGuildFieldWar dispatcher must execute in order.
enum class GuildFieldWarSideEffectKind : std::uint8_t {
    Drop,
    ForwardToMapServer,
    ForwardToMapServerNoUser,
    CheckGuildMasterLogin,
    CheckGuildFieldWarMoney,
    BroadcastToMapServersExceptSource,
    SendToTargetUserIfFound,
    AddGuildFieldWarMoney,
    BroadcastToAllUsers,
    ForwardToOriginatingClient,
};

struct GuildFieldWarSideEffect final {
    GuildFieldWarSideEffectKind kind = GuildFieldWarSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t dw_object_id = 0u;
    std::uint32_t dw_data = 0u;
    std::uint32_t dw_data1 = 0u;
    std::uint32_t dw_data2 = 0u;
    std::uint32_t target_user_conn = 0u;
    std::size_t broadcast_count = 0u;
    bool target_resolved = false;
    bool forward_payload = true;
};

struct GuildFieldWarSideEffectPlan final {
    std::vector<GuildFieldWarSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
    bool forward_to_map_server = false;
    bool check_guild_master_login = false;
    bool check_guild_field_war_money = false;
    bool broadcast_to_map_servers_except_source = false;
    bool send_to_target_user_if_found = false;
    bool add_guild_field_war_money = false;
    bool broadcast_to_all_users = false;
    bool forward_to_originating_client = false;
};

inline GuildFieldWarSideEffectPlan guild_fieldwar_user_side_effect_plan(
    const GuildFieldWarUserAction& a) {
    GuildFieldWarSideEffectPlan plan;
    using K = GuildFieldWarUserActionKind;
    using S = GuildFieldWarSideEffectKind;
    switch (a.kind) {
        case K::drop_no_user: {
            plan.drop = true;
            plan.effects.push_back({S::Drop, a.reply_protocol, a.dw_object_id, a.dw_data, a.dw_data1, a.dw_data2, 0u, 0u, false, false});
            return plan;
        }
        case K::forward_to_map_server_no_user: {
            plan.dispatched = true;
            plan.drop = false;
            plan.forward_to_map_server = true;
            plan.effects.push_back({S::ForwardToMapServerNoUser, a.reply_protocol, a.dw_object_id, a.dw_data, a.dw_data1, a.dw_data2, 0u, 0u, false, a.forward_payload});
            return plan;
        }
        case K::check_guild_master_login: {
            plan.dispatched = true;
            plan.drop = false;
            plan.check_guild_master_login = true;
            plan.effects.push_back({S::CheckGuildMasterLogin, a.reply_protocol, a.dw_object_id, a.dw_data, a.dw_data1, a.dw_data2, 0u, 0u, false, a.forward_payload});
            return plan;
        }
        case K::check_guild_field_war_money: {
            plan.dispatched = true;
            plan.drop = false;
            plan.check_guild_field_war_money = true;
            plan.effects.push_back({S::CheckGuildFieldWarMoney, a.reply_protocol, a.dw_object_id, a.dw_data, a.dw_data1, a.dw_data2, 0u, 0u, false, a.forward_payload});
            return plan;
        }
        case K::forward_to_map_server: {
            plan.dispatched = true;
            plan.drop = false;
            plan.forward_to_map_server = true;
            plan.effects.push_back({S::ForwardToMapServer, a.reply_protocol, a.dw_object_id, a.dw_data, a.dw_data1, a.dw_data2, 0u, 0u, false, a.forward_payload});
            return plan;
        }
    }
    return plan;
}

inline GuildFieldWarSideEffectPlan guild_fieldwar_server_side_effect_plan(
    const GuildFieldWarServerAction& a) {
    GuildFieldWarSideEffectPlan plan;
    using K = GuildFieldWarServerActionKind;
    using S = GuildFieldWarSideEffectKind;
    switch (a.kind) {
        case K::drop_no_user: {
            plan.drop = true;
            plan.effects.push_back({S::Drop, a.reply_protocol, a.dw_object_id, 0u, a.dw_data1, a.dw_data2, 0u, 0u, false, false});
            return plan;
        }
        case K::broadcast_to_map_servers_except_source: {
            plan.dispatched = true;
            plan.drop = false;
            plan.broadcast_to_map_servers_except_source = true;
            plan.effects.push_back({S::BroadcastToMapServersExceptSource, a.reply_protocol, a.dw_object_id, 0u, a.dw_data1, a.dw_data2, 0u, a.broadcast_count, false, a.forward_payload});
            return plan;
        }
        case K::send_to_target_user_if_found: {
            plan.dispatched = true;
            plan.drop = !a.target_resolved;
            plan.send_to_target_user_if_found = true;
            plan.effects.push_back({S::SendToTargetUserIfFound, a.reply_protocol, a.dw_object_id, 0u, a.dw_data1, a.dw_data2, a.target_user_conn, 0u, a.target_resolved, a.forward_payload});
            return plan;
        }
        case K::add_guild_field_war_money: {
            plan.dispatched = true;
            plan.drop = false;
            plan.add_guild_field_war_money = true;
            plan.effects.push_back({S::AddGuildFieldWarMoney, a.reply_protocol, a.dw_object_id, 0u, a.dw_data1, a.dw_data2, 0u, 0u, false, a.forward_payload});
            return plan;
        }
        case K::broadcast_to_all_users: {
            plan.dispatched = true;
            plan.drop = false;
            plan.broadcast_to_all_users = true;
            plan.effects.push_back({S::BroadcastToAllUsers, a.reply_protocol, a.dw_object_id, 0u, a.dw_data1, a.dw_data2, 0u, a.broadcast_count, false, a.forward_payload});
            return plan;
        }
        case K::forward_to_originating_client: {
            plan.dispatched = true;
            plan.drop = false;
            plan.forward_to_originating_client = true;
            plan.effects.push_back({S::ForwardToOriginatingClient, a.reply_protocol, a.dw_object_id, 0u, a.dw_data1, a.dw_data2, 0u, 0u, false, a.forward_payload});
            return plan;
        }
        case K::drop_unknown_protocol: {
            plan.drop = true;
            plan.effects.push_back({S::Drop, a.reply_protocol, a.dw_object_id, 0u, a.dw_data1, a.dw_data2, 0u, a.broadcast_count, false, false});
            return plan;
        }
    }
    return plan;
}

}  // namespace mxh::server

