// D4.170 -- AgentStreetStall side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_STREETSTALLUserMsgParser (lines 5083-5092). The data plane
// (classify_streetstall_user) decides the action; this header captures the ordered
// side-effect list the orchestrator must execute.
//
// Legacy semantics (preserved verbatim):
//   - find user by dwConnectionIndex; if missing -> drop silently.
//   - if pUserInfo->dwCharacterID != pmsg->dwObjectID -> drop silently.
//   - default -> TransToMapServerMsgParser.
//
// Note: MP_STREETSTALL is user-side only on the agent. Server-originated
// streetstall traffic reaches map servers via direct distribution, never
// through the agent category dispatch.
//
// Side effects:
//   - Drop: silent no-op (no network I/O, no map forward).
//   - ForwardToMapServer: route the original packet to the user map.

#pragma once

#include <cstdint>
#include <vector>

#include "mxh/server/agent_streetstall.hpp"

namespace mxh::server {

// Side-effect kinds the AgentStreetStall dispatcher must execute in order.
enum class StreetStallSideEffectKind : std::uint8_t {
    Drop,
    ForwardToMapServer,
};

struct StreetStallSideEffect final {
    StreetStallSideEffectKind kind = StreetStallSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t dw_object_id = 0u;
    bool forward_payload = false;
};

struct StreetStallSideEffectPlan final {
    std::vector<StreetStallSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
    bool forward_to_map_server = false;
};

inline StreetStallSideEffectPlan streetstall_user_side_effect_plan(
    const StreetStallUserAction& a) {
    StreetStallSideEffectPlan plan;
    using K = StreetStallUserActionKind;
    using S = StreetStallSideEffectKind;
    switch (a.kind) {
        case K::drop_no_user: {
            plan.drop = true;
            plan.effects.push_back({S::Drop, a.reply_protocol, a.dw_object_id, false});
            return plan;
        }
        case K::drop_object_id_mismatch: {
            plan.drop = true;
            plan.effects.push_back({S::Drop, a.reply_protocol, a.dw_object_id, false});
            return plan;
        }
        case K::forward_to_map_server: {
            plan.dispatched = true;
            plan.drop = false;
            plan.forward_to_map_server = true;
            plan.effects.push_back({S::ForwardToMapServer, a.reply_protocol, a.dw_object_id, a.forward_payload});
            return plan;
        }
    }
    return plan;
}

}  // namespace mxh::server

