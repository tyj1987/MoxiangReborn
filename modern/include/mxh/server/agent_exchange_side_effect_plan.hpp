// D4.171 -- AgentExchange side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_EXCHANGEUserMsgParser (lines 5095-5104). The data plane
// (classify_exchange_user) decides the action; this header captures the ordered
// side-effect list the orchestrator must execute.
//
// Legacy semantics (preserved verbatim):
//   - find user by dwConnectionIndex; if missing -> drop silently.
//   - if pUserInfo->dwCharacterID != pmsg->dwObjectID -> drop silently.
//   - default -> TransToMapServerMsgParser.
//
// Note: MP_EXCHANGE is user-side only on the agent. Exchange state changes
// flow map-server-side, never via the agent category dispatch. The data plane
// is intentionally a near-clone of MP_STREETSTALLUserMsgParser; both are simple
// user-found + objectid integrity gates.
//
// Side effects:
//   - Drop: silent no-op (no network I/O, no map forward).
//   - ForwardToMapServer: route the original packet to the user map.

#pragma once

#include <cstdint>
#include <vector>

#include "mxh/server/agent_exchange.hpp"

namespace mxh::server {

// Side-effect kinds the AgentExchange dispatcher must execute in order.
enum class ExchangeSideEffectKind : std::uint8_t {
    Drop,
    ForwardToMapServer,
};

struct ExchangeSideEffect final {
    ExchangeSideEffectKind kind = ExchangeSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t dw_object_id = 0u;
    bool forward_payload = false;
};

struct ExchangeSideEffectPlan final {
    std::vector<ExchangeSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
    bool forward_to_map_server = false;
};

inline ExchangeSideEffectPlan exchange_user_side_effect_plan(
    const ExchangeUserAction& a) {
    ExchangeSideEffectPlan plan;
    using K = ExchangeUserActionKind;
    using S = ExchangeSideEffectKind;
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

