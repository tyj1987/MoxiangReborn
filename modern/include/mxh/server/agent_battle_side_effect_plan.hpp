#pragma once

//
// agent_battle_side_effect_plan.hpp -- D4.121
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_BATTLE handling. The data plane (classify_battle) decides which action
// to take; this header captures the ordered side-effect list the orchestrator
// must execute. There is NO MP_BATTLE* function in the legacy AgentNetworkMsgParser
// (battle state lives on the map server) -- MP_BATTLE falls through to TransToMapServerMsgParser
// (default forward) for all 31 sub-protocols (battle_info..battle_vimu_waiting_cancel_nack).
//
// Legacy invariants (locked 1:1):
//   - classify_battle always returns forward_to_map for any protocol in [0, 30].
//   - battle_category = 31 (MP_BATTLE in MP_CATEGORY).
//   - No state mutation on agent side.
//
// Side effects:
//   - ForwardRawToMap: emit raw forward to user->dwMapServerConnectionIndex.
//   - Drop: silent no-op (defensive; never emitted by classify_battle today).

#include <cstdint>
#include <vector>

#include "mxh/server/agent_battle.hpp"

namespace mxh::server {

// Side-effect kinds the orchestrator must dispatch in order.
enum class AgentBattleSideEffectKind : std::uint8_t {
    Drop,                       // silent no-op
    ForwardRawToMap,            // raw forward to map server
};

struct AgentBattleSideEffect final {
    AgentBattleSideEffectKind kind = AgentBattleSideEffectKind::Drop;
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
};

struct AgentBattleSideEffectPlan final {
    std::vector<AgentBattleSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

// Predicate: does the effect target the map server?
inline bool agent_battle_effect_targets_map(const AgentBattleSideEffect& e) noexcept {
    return e.kind == AgentBattleSideEffectKind::ForwardRawToMap;
}

// Apply the side-effect plan. There is no agent-side state mutation for
// MP_BATTLE; the orchestrator only dispatches the forward action.
// Returns true if any side effect was dispatched; false if the plan was dropped.
inline bool apply_agent_battle_side_effect_plan(const AgentBattleSideEffectPlan& plan) {
    if (plan.drop || plan.effects.empty()) return false;
    bool applied = false;
    for (const auto& e : plan.effects) {
        switch (e.kind) {
            case AgentBattleSideEffectKind::Drop:
                break;
            case AgentBattleSideEffectKind::ForwardRawToMap:
                // Dispatcher-side: raw forward to map server.
                applied = true;
                break;
        }
    }
    return applied;
}

// Plan-builder: convert a data-plane BattleAction into a side-effect plan.
inline AgentBattleSideEffectPlan agent_battle_side_effect_plan(const BattleAction& a) {
    AgentBattleSideEffectPlan plan;
    using K = AgentBattleSideEffectKind;
    using A = BattleActionKind;
    switch (a.kind) {
        case A::forward_to_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToMap, a.protocol, a.object_id});
            return plan;
        case A::drop_protocol:
            // Defensive: never emitted by classify_battle today but kept for
            // forward compat with future protocol gating.
            plan.dispatched = false;
            plan.drop = true;
            plan.effects.push_back({K::Drop, a.protocol, a.object_id});
            return plan;
    }
    return plan;
}

// Mirror plan-builder: classify-and-build from a BattleRequest. This
// re-runs classify_battle and produces the canonical plan.
inline AgentBattleSideEffectPlan agent_battle_user_side_effect_plan(const BattleRequest& r) {
    const auto a = classify_battle(r);
    return agent_battle_side_effect_plan(a);
}

}  // namespace mxh::server

