#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/agent_userconn.hpp>

namespace mxh::server {

// Side-effect kinds the AgentUserConn dispatcher must execute in order.
enum class UserConnSideEffectKind : std::uint8_t {
    Drop,
    LogCheatUse,
    Send2User,
    ForwardToMapServer,
    Broadcast2MapServers,
    BroadcastExcept2MapServers,
    DisconnectUser,
    ReplyLogincheckDelete,
    SendNackToDist,
};

struct UserConnSideEffect final {
    UserConnSideEffectKind kind = UserConnSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t error_code = 0u;
    bool forward_payload = false;
    bool reset_connection_check_flag = false;
};

struct UserConnSideEffectPlan final {
    std::vector<UserConnSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
    bool forward_to_map = false;
    bool broadcast = false;
    bool broadcast_except = false;
    bool send_to_user = false;
    bool disconnect = false;
    bool log_cheat = false;
    bool reply_logincheck_delete = false;
};

inline UserConnSideEffectPlan userconn_side_effect_plan(
    const UserConnAction& action) {
    UserConnSideEffectPlan plan;
    using K = UserConnActionKind;
    using S = UserConnSideEffectKind;
    switch (action.kind) {
        case K::drop_no_user:
        case K::drop_unauthenticated:
            plan.drop = true;
            plan.effects.push_back({S::Drop, 0u, 0u, false, false});
            return plan;
        case K::send_nack_to_dist_no_agent:
            plan.dispatched = true;
            plan.drop = false;
            plan.send_to_user = true;
            plan.effects.push_back({S::SendNackToDist, action.reply_protocol, action.error_code, false, false});
            return plan;
        case K::send_notify_userlogin_ack:
            plan.dispatched = true;
            plan.drop = false;
            plan.send_to_user = true;
            plan.effects.push_back({S::Send2User, action.reply_protocol, 0u, action.forward_payload, false});
            return plan;
        case K::send_otheruser_connecttry_notify:
            plan.dispatched = true;
            plan.drop = false;
            plan.send_to_user = true;
            plan.effects.push_back({S::Send2User, action.reply_protocol, 0u, false, false});
            return plan;
        case K::send_disconnected_by_overlap_to_client:
            plan.dispatched = true;
            plan.drop = false;
            plan.send_to_user = true;
            plan.effects.push_back({S::Send2User, action.reply_protocol, 0u, false, false});
            return plan;
        case K::send_nowaitexit_to_map_server:
            plan.dispatched = true;
            plan.drop = false;
            plan.forward_to_map = true;
            plan.effects.push_back({S::ForwardToMapServer, action.reply_protocol, 0u, false, false});
            return plan;
        case K::send_to_user:
            plan.dispatched = true;
            plan.drop = false;
            plan.send_to_user = true;
            plan.effects.push_back({S::Send2User, action.reply_protocol, action.error_code, action.forward_payload, false});
            return plan;
        case K::forward_to_map_server:
            plan.dispatched = true;
            plan.drop = false;
            plan.forward_to_map = true;
            plan.effects.push_back({S::ForwardToMapServer, action.reply_protocol, 0u, action.forward_payload, action.disable_failure_flag});
            return plan;
        case K::broadcast_to_map_servers:
            plan.dispatched = true;
            plan.drop = false;
            plan.broadcast = true;
            plan.effects.push_back({S::Broadcast2MapServers, action.reply_protocol, 0u, action.forward_payload, false});
            return plan;
        case K::broadcast_except_to_map_servers:
            plan.dispatched = true;
            plan.drop = false;
            plan.broadcast_except = true;
            plan.effects.push_back({S::BroadcastExcept2MapServers, action.reply_protocol, 0u, action.forward_payload, false});
            return plan;
        case K::disconnect_user:
            plan.dispatched = true;
            plan.drop = false;
            plan.disconnect = true;
            if (action.reply_protocol != 0u) {
                plan.send_to_user = true;
                plan.effects.push_back({S::Send2User, action.reply_protocol, 0u, false, false});
            }
            plan.effects.push_back({S::DisconnectUser, 0u, 0u, false, false});
            return plan;
        case K::reply_logincheck_delete:
            plan.dispatched = true;
            plan.drop = false;
            plan.reply_logincheck_delete = true;
            plan.effects.push_back({S::ReplyLogincheckDelete, 0u, 0u, false, false});
            return plan;
        case K::log_cheat_use:
            plan.dispatched = true;
            plan.drop = false;
            plan.log_cheat = true;
            plan.effects.push_back({S::LogCheatUse, action.reply_protocol, 0u, false, false});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server
