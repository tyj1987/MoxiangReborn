
//
// D4.104 -- AgentFortWar side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_FORTWARServerMsgParser (lines 5290-5452). The data plane (classify_fortwar)
// decides which action to take; this header captures the ordered side-effect
// list the orchestrator must execute.
//
// Legacy branches:
//   - start_before10min/start/end (1,2,4): Broadcast2AllUser (raw to all clients).
//   - *_to_map (5,6,7,8): Broadcast2MapServerExceptOne (raw to all maps except sender).
//   - info/ing/default (0,3,other): if user_object_found, forward raw via
//     Send2User(user->dwConnectionIndex); else drop.
//

#include <cstdint>
#include <vector>

#include "mxh/server/agent_fortwar.hpp"

namespace mxh::server {

// SERVER side-effect kinds the orchestrator must dispatch in order.
enum class FortWarSideEffectKind : std::uint8_t {
    Drop,                                  // user not found in objectid table
    BroadcastToAllUsers,                   // raw to every connected user (g_Network.Broadcast2AllUser)
    BroadcastToOtherMaps,                  // raw to every map server except sender (g_Network.Broadcast2MapServerExceptOne)
    ForwardRawToUser,                      // raw to user->dwConnectionIndex
};

struct FortWarSideEffect final {
    FortWarSideEffectKind kind = FortWarSideEffectKind::Drop;
    std::uint8_t protocol = 0u;
};

struct FortWarSideEffectPlan final {
    std::vector<FortWarSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool fortwar_effect_targets_map(const FortWarSideEffect& e) noexcept {
    return e.kind == FortWarSideEffectKind::BroadcastToOtherMaps;
}

inline bool fortwar_effect_targets_user(const FortWarSideEffect& e) noexcept {
    return e.kind == FortWarSideEffectKind::ForwardRawToUser;
}

inline bool fortwar_effect_targets_all(const FortWarSideEffect& e) noexcept {
    return e.kind == FortWarSideEffectKind::BroadcastToAllUsers;
}

inline FortWarSideEffectPlan fortwar_side_effect_plan(const FortWarAction& a) {
    FortWarSideEffectPlan plan;
    using K = FortWarSideEffectKind;
    using A = FortWarActionKind;
    switch (a.kind) {
        case A::drop_no_user:
            plan.drop = true;
            plan.effects.push_back({K::Drop, a.protocol});
            return plan;
        case A::broadcast_to_all_users:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::BroadcastToAllUsers, a.protocol});
            return plan;
        case A::broadcast_to_other_maps:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::BroadcastToOtherMaps, a.protocol});
            return plan;
        case A::forward_to_user_if_found:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToUser, a.protocol});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server