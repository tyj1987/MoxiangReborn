
//
// D4.119 -- AgentUser state-side-effect plan.
//
// 1:1 port of legacy state mutations applied by [Server]Agent/AgentServer.cpp +
// AgentNetworkMsgParser.cpp. The data plane functions (insert_agent_user,
// remove_agent_user, assign_agent_user_map, toggle_agent_user_force_move)
// decide which action to take; this header captures the ordered state-side-effect
// list the orchestrator must apply.
//
// Legacy invariants (locked 1:1):
//   - InsertAgentUser: only succeeds if !r.in_use; sets dwAuthKey + dwObjectID + in_use=true.
//   - RemoveAgentUser: only succeeds if r.in_use; clears info + in_use=false.
//   - AssignAgentUserMap: only succeeds if channel != 0 (sentinel for unassigned);
//     sets dwMapChannel.
//   - ToggleAgentUserForceMove: flips bForceMove (0 <-> 1).
//
// State side effects:
//   - InsertAgentUser: in_use=true, dwAuthKey=key, dwObjectID=object_id.
//   - RemoveAgentUser: in_use=false, info reset to defaults.
//   - AssignAgentUserMap: dwMapChannel=channel.
//   - ToggleAgentUserForceMove: bForceMove ^= 1.
//   - Drop (precondition not met: already in_use / not in_use / channel==0): silent no-op.
//

#include <cstdint>
#include <vector>

#include "mxh/server/agent_user.hpp"

namespace mxh::server {

// State-side-effect kinds the orchestrator must apply in order.
enum class AgentUserSideEffectKind : std::uint8_t {
    Drop,                                  // precondition not met (silent no-op)
    InsertAgentUser,                       // in_use=true; set dwAuthKey + dwObjectID
    RemoveAgentUser,                       // in_use=false; clear info
    AssignAgentUserMap,                    // dwMapChannel=channel (only if channel!=0)
    ToggleAgentUserForceMove,              // bForceMove ^= 1
};

struct AgentUserSideEffect final {
    AgentUserSideEffectKind kind = AgentUserSideEffectKind::Drop;
    std::uint32_t auth_key = 0u;
    std::uint32_t object_id = 0u;
    std::uint32_t map_channel = 0u;
    std::uint8_t force_move = 0u;
    bool clear_info = false;
};

struct AgentUserSideEffectPlan final {
    std::vector<AgentUserSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

// Apply the side-effect plan to a record (1:1 mirror of the legacy mutators).
// Returns true if any state mutation happened; false if the plan was dropped.
inline bool apply_agent_user_side_effect_plan(AgentUserRecord& r, const AgentUserSideEffectPlan& plan) {
    if (plan.drop || plan.effects.empty()) return false;
    bool applied = false;
    for (const auto& e : plan.effects) {
        switch (e.kind) {
            case AgentUserSideEffectKind::Drop:
                break;
            case AgentUserSideEffectKind::InsertAgentUser:
                if (r.in_use) break;
                r.info.dwAuthKey = e.auth_key;
                r.info.dwObjectID = e.object_id;
                r.in_use = true;
                applied = true;
                break;
            case AgentUserSideEffectKind::RemoveAgentUser:
                if (!r.in_use) break;
                r.info = AgentUserInfo{};
                r.in_use = false;
                applied = true;
                break;
            case AgentUserSideEffectKind::AssignAgentUserMap:
                if (e.map_channel == 0u) break;
                r.info.dwMapChannel = e.map_channel;
                applied = true;
                break;
            case AgentUserSideEffectKind::ToggleAgentUserForceMove:
                r.info.bForceMove = r.info.bForceMove ? 0u : 1u;
                applied = true;
                break;
        }
    }
    return applied;
}

// Build side-effect plans from the legacy classifier functions.
inline AgentUserSideEffectPlan agent_user_insert_side_effect_plan(std::uint32_t key, std::uint32_t object_id) {
    AgentUserSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    plan.effects.push_back({AgentUserSideEffectKind::InsertAgentUser, key, object_id, 0u, 0u, false});
    return plan;
}

inline AgentUserSideEffectPlan agent_user_remove_side_effect_plan() {
    AgentUserSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    plan.effects.push_back({AgentUserSideEffectKind::RemoveAgentUser, 0u, 0u, 0u, 0u, true});
    return plan;
}

inline AgentUserSideEffectPlan agent_user_assign_map_side_effect_plan(std::uint32_t channel) {
    AgentUserSideEffectPlan plan;
    if (channel == 0u) {
        plan.drop = true;
        plan.effects.push_back({AgentUserSideEffectKind::Drop, 0u, 0u, 0u, 0u, false});
        return plan;
    }
    plan.dispatched = true;
    plan.drop = false;
    plan.effects.push_back({AgentUserSideEffectKind::AssignAgentUserMap, 0u, 0u, channel, 0u, false});
    return plan;
}

inline AgentUserSideEffectPlan agent_user_toggle_force_move_side_effect_plan() {
    AgentUserSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    plan.effects.push_back({AgentUserSideEffectKind::ToggleAgentUserForceMove, 0u, 0u, 0u, 0u, false});
    return plan;
}

}  // namespace mxh::server