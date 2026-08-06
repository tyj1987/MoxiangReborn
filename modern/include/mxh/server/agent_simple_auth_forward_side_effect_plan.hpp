#pragma once

//
// agent_simple_auth_forward_side_effect_plan.hpp -- D4.122
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_STREETSTALLUserMsgParser + MP_EXCHANGEUserMsgParser. Both share the same
// auth gate: drop when user not found OR character_id mismatch, else forward to map.
//
// Legacy invariants (locked 1:1):
//   - Streetstall category = 29 (MP_STREETSTALL).
//   - Exchange category = 28 (MP_EXCHANGE).
//   - classify_streetstall_user / classify_exchange_user both apply:
//       if (!user_found)                -> drop_no_user
//       if (character_id != object_id)  -> drop_object_mismatch
//       else                            -> forward_to_map
//   - No state mutation on agent side.
//
// Side effects:
//   - ForwardRawToMap: raw forward to user->dwMapServerConnectionIndex.
//   - Drop: silent no-op (protocol != 0 sentinel for dropped paths).

#include <cstdint>
#include <vector>

#include "mxh/server/agent_simple_auth_forward.hpp"

namespace mxh::server {

// Side-effect kinds the orchestrator must dispatch in order.
enum class AgentSimpleAuthForwardSideEffectKind : std::uint8_t {
    Drop,                       // silent no-op
    ForwardRawToMap,            // raw forward to map server
};

struct AgentSimpleAuthForwardSideEffect final {
    AgentSimpleAuthForwardSideEffectKind kind = AgentSimpleAuthForwardSideEffectKind::Drop;
    std::uint32_t connection_index = 0u;
    std::uint8_t protocol = 0u;
    std::uint8_t category = 0u;  // MP_STREETSTALL (29) or MP_EXCHANGE (28)
};

struct AgentSimpleAuthForwardSideEffectPlan final {
    std::vector<AgentSimpleAuthForwardSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

// Predicate: does the effect target the map server?
inline bool agent_simple_auth_forward_effect_targets_map(
        const AgentSimpleAuthForwardSideEffect& e) noexcept {
    return e.kind == AgentSimpleAuthForwardSideEffectKind::ForwardRawToMap;
}

// Apply the side-effect plan. No agent-side state mutation; the
// orchestrator only dispatches the forward action.
inline bool apply_agent_simple_auth_forward_side_effect_plan(
        const AgentSimpleAuthForwardSideEffectPlan& plan) {
    if (plan.drop || plan.effects.empty()) return false;
    bool applied = false;
    for (const auto& e : plan.effects) {
        switch (e.kind) {
            case AgentSimpleAuthForwardSideEffectKind::Drop:
                break;
            case AgentSimpleAuthForwardSideEffectKind::ForwardRawToMap:
                applied = true;
                break;
        }
    }
    return applied;
}

// Generic plan-builder: convert a 3-state action into a plan. Both StreetStall
// and Exchange share this shape: 3 action kinds mapping to 2 effect kinds.
inline AgentSimpleAuthForwardSideEffectPlan agent_simple_auth_forward_side_effect_plan(
        std::uint32_t connection_index, std::uint8_t protocol,
        std::uint8_t category, std::uint8_t action_kind_int) {
    AgentSimpleAuthForwardSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    if (action_kind_int == 0) {  // forward_to_map
        plan.effects.push_back({AgentSimpleAuthForwardSideEffectKind::ForwardRawToMap,
                                connection_index, protocol, category});
    } else {  // drop_no_user or drop_object_mismatch
        plan.drop = true;
        plan.effects.push_back({AgentSimpleAuthForwardSideEffectKind::Drop,
                                connection_index, 0u, category});
    }
    return plan;
}

// Convenience builders that wrap the generic builder with category + classifier.
inline AgentSimpleAuthForwardSideEffectPlan agent_streetstall_user_side_effect_plan(
        const StreetStallUserAction& a) {
    return agent_simple_auth_forward_side_effect_plan(
        a.connection_index, a.protocol, streetstall_category,
        static_cast<std::uint8_t>(a.kind));
}

inline AgentSimpleAuthForwardSideEffectPlan agent_streetstall_user_side_effect_plan(
        const StreetStallUserRequest& r) {
    const auto a = classify_streetstall_user(r);
    return agent_streetstall_user_side_effect_plan(a);
}

inline AgentSimpleAuthForwardSideEffectPlan agent_exchange_user_side_effect_plan(
        const ExchangeUserAction& a) {
    return agent_simple_auth_forward_side_effect_plan(
        a.connection_index, a.protocol, exchange_category,
        static_cast<std::uint8_t>(a.kind));
}

inline AgentSimpleAuthForwardSideEffectPlan agent_exchange_user_side_effect_plan(
        const ExchangeUserRequest& r) {
    const auto a = classify_exchange_user(r);
    return agent_exchange_user_side_effect_plan(a);
}

}  // namespace mxh::server

