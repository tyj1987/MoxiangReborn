#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/agent_cheat.hpp>

namespace mxh::server {

// Side-effect intents that an AgentCheat dispatcher must execute in order.
enum class CheatUserSideEffectKind : std::uint8_t {
    LogGMToolUse,
    LogGMRecord,
    Send2User,
    Broadcast2MapServer,
    Trans2MapServer,
    Drop,
};

struct CheatUserSideEffect final {
    CheatUserSideEffectKind kind = CheatUserSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    bool forward_payload = false;
    bool broadcast_all_maps = false;
};

struct CheatUserSideEffectPlan final {
    std::vector<CheatUserSideEffect> effects;
    bool dispatched = false;
    bool logged = false;
    bool forward_to_map = false;
    bool broadcast = false;
    bool send_to_user = false;
    bool drop = true;
};

inline CheatUserSideEffectPlan cheat_user_side_effect_plan(
    const CheatUserAction& action) {
    CheatUserSideEffectPlan plan;
    using K = CheatUserActionKind;
    using S = CheatUserSideEffectKind;
    switch (action.kind) {
        case K::drop_unauthorized:
        case K::drop_no_user:
        case K::drop_gm_not_logged_in:
        case K::drop_gm_blocked_subprotocol:
            plan.drop = true;
            plan.effects.push_back({S::Drop, 0u, false, false});
            return plan;
        case K::gm_login:
            plan.dispatched = true;
            plan.logged = true;
            plan.drop = false;
            plan.effects.push_back({S::LogGMRecord, action.reply_protocol, true, false});
            return plan;
        case K::send_bancharacter_to_map_server:
            plan.dispatched = true;
            plan.broadcast = true;
            plan.drop = false;
            plan.effects.push_back({S::Send2User, 0u, false, false});
            plan.effects.push_back({S::Broadcast2MapServer,
                action.reply_protocol, action.forward_payload, true});
            return plan;
        case K::send_bancharacter_nack_to_user:
            plan.dispatched = true;
            plan.send_to_user = true;
            plan.drop = false;
            plan.effects.push_back({S::Send2User, action.reply_protocol, false, false});
            return plan;
        case K::send_blockcharacter_syn:
            plan.dispatched = true;
            plan.broadcast = true;
            plan.drop = false;
            plan.effects.push_back({S::Broadcast2MapServer,
                action.reply_protocol, action.forward_payload, true});
            return plan;
        case K::send_event_monster_regen:
            plan.dispatched = true;
            plan.broadcast = true;
            plan.drop = false;
            plan.effects.push_back({S::Broadcast2MapServer,
                action.reply_protocol, action.forward_payload, true});
            return plan;
        case K::send_event_monster_delete:
            plan.dispatched = true;
            plan.broadcast = true;
            plan.drop = false;
            plan.effects.push_back({S::Broadcast2MapServer,
                action.reply_protocol, action.forward_payload, true});
            return plan;
        case K::send_pkallow_syn:
            plan.dispatched = true;
            plan.broadcast = true;
            plan.drop = false;
            plan.effects.push_back({S::Broadcast2MapServer,
                action.reply_protocol, action.forward_payload, true});
            return plan;
        case K::send_changemap_nack_to_user:
            plan.dispatched = true;
            plan.send_to_user = true;
            plan.drop = false;
            plan.effects.push_back({S::Send2User, action.reply_protocol, false, false});
            return plan;
        case K::log_gm_tool_use:
            plan.dispatched = true;
            plan.logged = true;
            plan.drop = false;
            if (action.broadcast_all_maps) {
                plan.broadcast = true;
                plan.effects.push_back({S::LogGMToolUse, action.reply_protocol, false, false});
                plan.effects.push_back({S::Broadcast2MapServer,
                    action.reply_protocol, action.forward_payload, true});
            } else {
                plan.forward_to_map = true;
                plan.effects.push_back({S::LogGMToolUse, action.reply_protocol, false, false});
                plan.effects.push_back({S::Trans2MapServer,
                    action.reply_protocol, action.forward_payload, false});
            }
            return plan;
        case K::trans_to_map_server:
            plan.dispatched = true;
            plan.drop = false;
            if (action.broadcast_all_maps) {
                plan.broadcast = true;
                plan.effects.push_back({S::Broadcast2MapServer,
                    action.reply_protocol, action.forward_payload, true});
            } else {
                plan.forward_to_map = true;
                plan.effects.push_back({S::Trans2MapServer,
                    action.reply_protocol, action.forward_payload, false});
            }
            return plan;
        case K::forward_to_user:
            plan.dispatched = true;
            plan.send_to_user = true;
            plan.drop = false;
            plan.effects.push_back({S::Send2User, action.reply_protocol, true, false});
            return plan;
    }
    return plan;
}

struct CheatServerSideEffect final {
    CheatUserSideEffectKind kind = CheatUserSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    bool forward_payload = false;
    bool broadcast_all_maps = false;
};

struct CheatServerSideEffectPlan final {
    std::vector<CheatServerSideEffect> effects;
    bool dispatched = false;
    bool forward_to_map = false;
    bool broadcast = false;
    bool send_to_user = false;
    bool drop = true;
};

inline CheatServerSideEffectPlan cheat_server_side_effect_plan(
    const CheatServerAction& action) {
    CheatServerSideEffectPlan plan;
    using K = CheatServerActionKind;
    using S = CheatUserSideEffectKind;
    switch (action.kind) {
        case K::drop_unknown_char:
        case K::send_whereis_nack_to_user:
            if (action.kind == K::send_whereis_nack_to_user) {
                plan.dispatched = true;
                plan.send_to_user = true;
                plan.drop = false;
                plan.effects.push_back({S::Send2User, action.reply_protocol, false, false});
            } else {
                plan.drop = true;
                plan.effects.push_back({S::Drop, 0u, false, false});
            }
            return plan;
        case K::send_bancharacter_to_user:
        case K::send_bancharacter_ack_to_user:
        case K::send_whereis_ack_to_user:
            plan.dispatched = true;
            plan.send_to_user = true;
            plan.drop = false;
            plan.effects.push_back({S::Send2User, action.reply_protocol, action.forward_payload, false});
            return plan;
        case K::send_bancharacter_ack_broadcast:
        case K::send_whereis_ack_broadcast:
            plan.dispatched = true;
            plan.broadcast = true;
            plan.drop = false;
            plan.effects.push_back({S::Broadcast2MapServer,
                action.reply_protocol, action.forward_payload, true});
            return plan;
        case K::trans_to_map_server:
            plan.dispatched = true;
            plan.drop = false;
            if (action.broadcast_all_maps) {
                plan.broadcast = true;
                plan.effects.push_back({S::Broadcast2MapServer,
                    action.reply_protocol, action.forward_payload, true});
            } else {
                plan.forward_to_map = true;
                plan.effects.push_back({S::Trans2MapServer,
                    action.reply_protocol, action.forward_payload, false});
            }
            return plan;
    }
    return plan;
}

}  // namespace mxh::server
