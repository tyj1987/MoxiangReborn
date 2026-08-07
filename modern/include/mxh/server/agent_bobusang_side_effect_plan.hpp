// D4.168 -- AgentBobusang side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_BOBUSANGUserMsgParser (lines 5191-5210) and MP_BOBUSANGServerMsgParser
// (lines 5212-5234). The data plane (classify_bobusang_user /
// classify_bobusang_server) decides the action; this header captures the ordered
// side-effect list the orchestrator must execute.
//
// Legacy semantics (preserved verbatim):
//   USER side:
//     drop_no_user        -> return (drop).
//     drop_gm_overshoot   -> return (drop) when user is GM and gmPower > MASTER.
//     forward_to_map_server -> TransToMapServerMsgParser.
//   SERVER side:
//     set_channel_state (APPEAR/DISAPPEAR_MAP_TO_AGENT) ->
//         BOBUSANGMGR->SetChannelState(dwData, state).
//     forward_to_originating_client -> TransToClientMsgParser.
//     drop_unknown_protocol -> silent no-op.
//
// Side effects:
//   - Drop: silent no-op (no network I/O, no BOBUSANGMGR touch).
//   - ForwardToMapServer: route the original packet to the originating user map.
//   - SetChannelState: BOBUSANGMGR->SetChannelState(channel_id, state).
//   - ForwardToOriginatingClient: route the original packet back to the user.

#pragma once

#include <cstdint>
#include <vector>

#include "mxh/server/agent_bobusang.hpp"

namespace mxh::server {

// Side-effect kinds the AgentBobusang dispatcher must execute in order.
enum class BobusangSideEffectKind : std::uint8_t {
    Drop,
    ForwardToMapServer,
    SetChannelState,
    ForwardToOriginatingClient,
};

struct BobusangSideEffect final {
    BobusangSideEffectKind kind = BobusangSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t dw_object_id = 0u;
    std::uint32_t channel_id = 0u;
    BobusangChannelState state = BobusangChannelState::Appear;
    bool forward_payload = true;
};

struct BobusangSideEffectPlan final {
    std::vector<BobusangSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
    bool forward_to_map_server = false;
    bool set_channel_state = false;
    bool forward_to_originating_client = false;
};

inline BobusangSideEffectPlan bobusang_user_side_effect_plan(
    const BobusangUserAction& a) {
    BobusangSideEffectPlan plan;
    using K = BobusangUserActionKind;
    using S = BobusangSideEffectKind;
    switch (a.kind) {
        case K::drop_no_user: {
            plan.drop = true;
            plan.effects.push_back({S::Drop, a.reply_protocol, a.dw_object_id, 0u, BobusangChannelState::Appear, false});
            return plan;
        }
        case K::drop_gm_overshoot: {
            plan.drop = true;
            plan.effects.push_back({S::Drop, a.reply_protocol, a.dw_object_id, 0u, BobusangChannelState::Appear, false});
            return plan;
        }
        case K::forward_to_map_server: {
            plan.dispatched = true;
            plan.drop = false;
            plan.forward_to_map_server = true;
            plan.effects.push_back({S::ForwardToMapServer, a.reply_protocol, a.dw_object_id, 0u, BobusangChannelState::Appear, a.forward_payload});
            return plan;
        }
    }
    return plan;
}

inline BobusangSideEffectPlan bobusang_server_side_effect_plan(
    const BobusangServerAction& a) {
    BobusangSideEffectPlan plan;
    using K = BobusangServerActionKind;
    using S = BobusangSideEffectKind;
    switch (a.kind) {
        case K::set_channel_state: {
            plan.dispatched = true;
            plan.drop = false;
            plan.set_channel_state = true;
            plan.effects.push_back({S::SetChannelState, a.reply_protocol, a.dw_object_id, a.channel_id, a.state, a.forward_payload});
            return plan;
        }
        case K::forward_to_originating_client: {
            plan.dispatched = true;
            plan.drop = false;
            plan.forward_to_originating_client = true;
            plan.effects.push_back({S::ForwardToOriginatingClient, a.reply_protocol, a.dw_object_id, a.channel_id, a.state, a.forward_payload});
            return plan;
        }
        case K::drop_unknown_protocol: {
            plan.drop = true;
            plan.effects.push_back({S::Drop, a.reply_protocol, a.dw_object_id, a.channel_id, a.state, false});
            return plan;
        }
    }
    return plan;
}

}  // namespace mxh::server

