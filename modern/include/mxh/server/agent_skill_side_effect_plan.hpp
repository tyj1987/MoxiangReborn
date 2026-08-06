//
// agent_skill_side_effect_plan.hpp -- D4.120
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_SkillUserMsgParser / MP_SkillServerMsgParser. The data plane functions
// (process_agent_skill_user / process_agent_skill_server / process_agent_skill_other)
// decide which action to take; this header captures the ordered state +
// side-effect list the orchestrator must execute.
//
// Legacy invariants (locked 1:1):
//   - User path: AddSkillUse(force=false) returns true -> forward to map;
//     returns false -> send_start_nack (Category=22, Protocol=2).
//   - Server path: AddSkillUse(force=true) always returns true -> forward to map.
//   - Other path: forward to map unconditionally (no delay state).
//   - Skill delay state mutation (SkillDelayManager) IS part of the side
//     effects: even when a NACK is dispatched, the delay table is NOT
//     touched (legacy AddSkillUse returns false without modifying state).
//
// State side effects:
//   - ApplySkillDelay: SkillDelayManager.AddSkillUse(force based on kind).
//   - ForwardRawToMap: emit raw forward to map server.
//   - SendStartNackToUser: emit MSG_BYTE (Protocol=2, bData=0) to user.
//   - Drop: precondition not met (silent no-op).

#include <cstdint>
#include <vector>

#include "mxh/server/agent_skill.hpp"
#include "mxh/server/skill_delay_manager.hpp"

namespace mxh::server {

// Side-effect kinds the orchestrator must apply in order.
enum class AgentSkillSideEffectKind : std::uint8_t {
    Drop,                                    // precondition not met (silent no-op)
    ApplySkillDelayUser,                      // SkillDelayManager.AddSkillUse(force=false)
    ApplySkillDelayServer,                    // SkillDelayManager.AddSkillUse(force=true)
    ForwardRawToMap,                          // raw forward to map server
    SendStartNackToUser,                      // MSG_BYTE (Protocol=2, bData=0) to user
};

struct AgentSkillSideEffect final {
    AgentSkillSideEffectKind kind = AgentSkillSideEffectKind::Drop;
    std::uint32_t character_id = 0u;
    std::uint32_t skill_index = 0u;
    std::uint32_t now_ms = 0u;
    bool skill_allowed = false;     // result of AddSkillUse (for 1:1 mirror test)
};

struct AgentSkillSideEffectPlan final {
    std::vector<AgentSkillSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

// Predicate: does the effect target the map server?
inline bool agent_skill_effect_targets_map(const AgentSkillSideEffect& e) noexcept {
    return e.kind == AgentSkillSideEffectKind::ForwardRawToMap;
}

// Predicate: does the effect target the originating user?
inline bool agent_skill_effect_targets_user(const AgentSkillSideEffect& e) noexcept {
    return e.kind == AgentSkillSideEffectKind::SendStartNackToUser;
}

// Predicate: does the effect mutate SkillDelayManager state?
inline bool agent_skill_effect_mutates_state(const AgentSkillSideEffect& e) noexcept {
    return e.kind == AgentSkillSideEffectKind::ApplySkillDelayUser
        || e.kind == AgentSkillSideEffectKind::ApplySkillDelayServer;
}

// Apply the side-effect plan to a SkillDelayManager. 1:1 mirror of legacy
// process_agent_skill_* + MP_SkillUserMsgParser / MP_SkillServerMsgParser.
// Returns true if any state mutation happened; false if the plan was dropped.
inline bool apply_agent_skill_side_effect_plan(SkillDelayManager& mgr,
                                              const AgentSkillSideEffectPlan& plan) {
    if (plan.drop || plan.effects.empty()) return false;
    bool applied = false;
    for (const auto& e : plan.effects) {
        switch (e.kind) {
            case AgentSkillSideEffectKind::Drop:
                break;
            case AgentSkillSideEffectKind::ApplySkillDelayUser:
                // Mirrors legacy AddSkillUse(force=false).
                add_skill_use(mgr, e.character_id, e.skill_index, e.now_ms, false);
                applied = true;
                break;
            case AgentSkillSideEffectKind::ApplySkillDelayServer:
                // Mirrors legacy AddSkillUse(force=true).
                add_skill_use(mgr, e.character_id, e.skill_index, e.now_ms, true);
                applied = true;
                break;
            case AgentSkillSideEffectKind::ForwardRawToMap:
                // Dispatcher-side: raw forward to map server.
                applied = true;
                break;
            case AgentSkillSideEffectKind::SendStartNackToUser:
                // Dispatcher-side: send NACK back to user.
                applied = true;
                break;
        }
    }
    return applied;
}

// Plan-builder: convert a data-plane AgentSkillAction into a side-effect plan.
inline AgentSkillSideEffectPlan agent_skill_side_effect_plan(const AgentSkillAction& a) {
    AgentSkillSideEffectPlan plan;
    using K = AgentSkillSideEffectKind;
    using A = AgentSkillActionKind;
    switch (a.kind) {
        case A::forward_to_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToMap, a.character_id, a.skill_index, 0u, true});
            return plan;
        case A::send_start_nack:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendStartNackToUser, a.character_id, a.skill_index, 0u, false});
            return plan;
    }
    return plan;
}

// Mirror plan-builder: classify-and-build a side-effect plan from a tuple
// (character_id, skill_index, now_ms) + SkillDelayManager reference. This
// re-runs the data-plane decision and produces a fully-mapped plan.
inline AgentSkillSideEffectPlan agent_skill_user_side_effect_plan(
        SkillDelayManager& mgr, std::uint32_t character_id,
        std::uint32_t skill_index, std::uint32_t now_ms) {
    AgentSkillSideEffectPlan plan;
    const bool allowed = add_skill_use(mgr, character_id, skill_index, now_ms, false);
    plan.dispatched = true;
    plan.drop = false;
    if (allowed) {
        plan.effects.push_back({AgentSkillSideEffectKind::ApplySkillDelayUser,
                                character_id, skill_index, now_ms, true});
        plan.effects.push_back({AgentSkillSideEffectKind::ForwardRawToMap,
                                character_id, skill_index, now_ms, true});
    } else {
        plan.effects.push_back({AgentSkillSideEffectKind::SendStartNackToUser,
                                character_id, skill_index, now_ms, false});
    }
    return plan;
}

inline AgentSkillSideEffectPlan agent_skill_server_side_effect_plan(
        SkillDelayManager& mgr, std::uint32_t character_id,
        std::uint32_t skill_index, std::uint32_t now_ms) {
    AgentSkillSideEffectPlan plan;
    add_skill_use(mgr, character_id, skill_index, now_ms, true);
    plan.dispatched = true;
    plan.drop = false;
    plan.effects.push_back({AgentSkillSideEffectKind::ApplySkillDelayServer,
                            character_id, skill_index, now_ms, true});
    plan.effects.push_back({AgentSkillSideEffectKind::ForwardRawToMap,
                            character_id, skill_index, now_ms, true});
    return plan;
}

inline AgentSkillSideEffectPlan agent_skill_other_side_effect_plan(
        std::uint32_t character_id, std::uint32_t skill_index) {
    AgentSkillSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    plan.effects.push_back({AgentSkillSideEffectKind::ForwardRawToMap,
                            character_id, skill_index, 0u, true});
    return plan;
}

}  // namespace mxh::server

