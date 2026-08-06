#pragma once

//
// agent_quest_side_effect_plan.hpp -- D4.123
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_QUEST handling. MP_QUEST is pass-through to map server (quest state lives on map).
// No MP_QUEST* function in the legacy AgentNetworkMsgParser -- MP_QUEST falls through
// to TransToMapServerMsgParser (default forward) for all 28 sub-protocols.
//
// Legacy invariants (locked 1:1):
//   - classify_quest always returns forward_to_map for any protocol.
//   - quest_category = 38 (MP_QUEST in MP_CATEGORY).
//   - No state mutation on agent side.
//
// Side effects:
//   - ForwardRawToMap: emit raw forward to user->dwMapServerConnectionIndex.
//   - Drop: silent no-op (defensive; never emitted by classify_quest today).

#include <cstdint>
#include <vector>

#include "mxh/server/agent_quest.hpp"

namespace mxh::server {

// Side-effect kinds the orchestrator must dispatch in order.
enum class AgentQuestSideEffectKind : std::uint8_t {
    Drop,                       // silent no-op
    ForwardRawToMap,            // raw forward to map server
};

struct AgentQuestSideEffect final {
    AgentQuestSideEffectKind kind = AgentQuestSideEffectKind::Drop;
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    std::uint8_t error_code = 0u;
};

struct AgentQuestSideEffectPlan final {
    std::vector<AgentQuestSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

// Predicate: does the effect target the map server?
inline bool agent_quest_effect_targets_map(const AgentQuestSideEffect& e) noexcept {
    return e.kind == AgentQuestSideEffectKind::ForwardRawToMap;
}

// Apply the side-effect plan. No agent-side state mutation for MP_QUEST;
// the orchestrator only dispatches the forward action.
inline bool apply_agent_quest_side_effect_plan(const AgentQuestSideEffectPlan& plan) {
    if (plan.drop || plan.effects.empty()) return false;
    bool applied = false;
    for (const auto& e : plan.effects) {
        switch (e.kind) {
            case AgentQuestSideEffectKind::Drop:
                break;
            case AgentQuestSideEffectKind::ForwardRawToMap:
                applied = true;
                break;
        }
    }
    return applied;
}

// Plan-builder: convert a data-plane QuestAction into a side-effect plan.
inline AgentQuestSideEffectPlan agent_quest_side_effect_plan(const QuestAction& a) {
    AgentQuestSideEffectPlan plan;
    using K = AgentQuestSideEffectKind;
    using A = QuestActionKind;
    switch (a.kind) {
        case A::forward_to_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToMap, a.protocol, a.object_id, a.error_code});
            return plan;
        case A::send_nack_no_quest:
            // Defensive: never emitted by classify_quest today but kept for
            // forward compat with future quest-gating logic.
            plan.dispatched = false;
            plan.drop = true;
            plan.effects.push_back({K::Drop, a.protocol, a.object_id, a.error_code});
            return plan;
    }
    return plan;
}

// Mirror plan-builder: classify-and-build from a QuestRequest.
inline AgentQuestSideEffectPlan agent_quest_user_side_effect_plan(const QuestRequest& r) {
    const auto a = classify_quest(r);
    return agent_quest_side_effect_plan(a);
}

}  // namespace mxh::server

