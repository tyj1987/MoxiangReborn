#pragma once

//
// agent_gtournament_side_effect_plan.hpp -- D4.124
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_GTOURNAMENTUserMsgParser (lines 4286-4295). Server-side (D4.103) is separate.
//
// Legacy invariants (locked 1:1):
//   - gtournament_category = 59 (MP_GTOURNAMENT).
//   - gt_map_num = 28 (the tournament map port).
//   - Routing matrix (legacy classify_gtournament_user):
//       if (!user_found)                                  -> drop_no_user
//       movetobattlemap_syn + user_map_found              -> send_movetobattle_to_user_map
//       movetobattlemap_syn + !user_map_found            -> send_movetobattle_nack_to_user
//       standinginfo_syn + gt_map_found                  -> send_standing_info_to_gt_map (target=gt_map)
//       standinginfo_syn + !gt_map_found                 -> send_standing_info_nack_to_user
//       battlejoin_syn + gt_map_found                    -> send_standing_info_to_gt_map (target=gt_map)
//       battlejoin_syn + !gt_map_found                   -> send_battlejoin_nack_to_user
//       observerjoin_syn + gt_map_found                  -> send_standing_info_to_gt_map (target=gt_map)
//       observerjoin_syn + !gt_map_found                 -> send_battlejoin_nack_to_user
//       leave_syn                                         -> send_leave_syn_to_user_map
//       cheat + cheat_data==1                            -> send_cheat_to_user_map
//       cheat + cheat_data!=1 + gt_map_found             -> send_cheat_to_gt_map
//       cheat + cheat_data!=1 + !gt_map_found            -> drop_no_user
//       event_start/event_end + user_level>8             -> drop_no_user
//       event_start/event_end + gt_map_found             -> send_event_to_gt_map
//       event_start/event_end + !gt_map_found            -> drop_no_user
//       default                                           -> forward_to_map_server
//
// Side effects:
//   - ForwardRawToMap: raw forward to map server (target_map=0 means general forwarding).
//   - SendNackToUser: emit MP_GTOURNAMENT_*_NACK to user.
//   - Drop: silent no-op.

#include <cstdint>
#include <vector>

#include "mxh/server/agent_gtournament.hpp"

namespace mxh::server {

// Side-effect kinds the orchestrator must dispatch in order.
enum class AgentGtournamentSideEffectKind : std::uint8_t {
    Drop,                              // silent no-op
    ForwardRawToMap,                   // raw forward (gt_map or user_map)
    SendNackToUser,                    // MP_GTOURNAMENT_*_NACK to user
};

struct AgentGtournamentSideEffect final {
    AgentGtournamentSideEffectKind kind = AgentGtournamentSideEffectKind::Drop;
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
    std::uint32_t error_code = 0u;
    std::uint16_t target_map = 0u;
};

struct AgentGtournamentSideEffectPlan final {
    std::vector<AgentGtournamentSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool agent_gtournament_effect_targets_map(const AgentGtournamentSideEffect& e) noexcept {
    return e.kind == AgentGtournamentSideEffectKind::ForwardRawToMap;
}

inline bool agent_gtournament_effect_targets_user(const AgentGtournamentSideEffect& e) noexcept {
    return e.kind == AgentGtournamentSideEffectKind::SendNackToUser;
}

inline bool apply_agent_gtournament_side_effect_plan(const AgentGtournamentSideEffectPlan& plan) {
    if (plan.drop || plan.effects.empty()) return false;
    bool applied = false;
    for (const auto& e : plan.effects) {
        switch (e.kind) {
            case AgentGtournamentSideEffectKind::Drop:
                break;
            case AgentGtournamentSideEffectKind::ForwardRawToMap:
                applied = true;
                break;
            case AgentGtournamentSideEffectKind::SendNackToUser:
                applied = true;
                break;
        }
    }
    return applied;
}

// Plan-builder: convert a GtournamentAction into a side-effect plan.
inline AgentGtournamentSideEffectPlan agent_gtournament_side_effect_plan(const GtournamentAction& a) {
    AgentGtournamentSideEffectPlan plan;
    using K = AgentGtournamentSideEffectKind;
    using A = GtournamentActionKind;
    switch (a.kind) {
        case A::forward_to_map_server:
        case A::send_movetobattle_to_user_map:
        case A::send_standing_info_to_gt_map:
        case A::send_leave_syn_to_user_map:
        case A::send_cheat_to_user_map:
        case A::send_cheat_to_gt_map:
        case A::send_event_to_gt_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToMap, a.protocol, a.object_id, 0u, a.target_map});
            return plan;
        case A::send_battlejoin_nack_to_user:
        case A::send_standing_info_nack_to_user:
        case A::send_movetobattle_nack_to_user:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendNackToUser, a.protocol, a.object_id, a.error_code, 0u});
            return plan;
        case A::drop_no_user:
            plan.dispatched = false;
            plan.drop = true;
            plan.effects.push_back({K::Drop, a.protocol, a.object_id, 0u, 0u});
            return plan;
    }
    return plan;
}

// Mirror plan-builder: classify-and-build from a GtournamentRequest.
inline AgentGtournamentSideEffectPlan agent_gtournament_user_side_effect_plan(const GtournamentRequest& r) {
    const auto a = classify_gtournament_user(r);
    return agent_gtournament_side_effect_plan(a);
}

}  // namespace mxh::server

