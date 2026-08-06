
//
// D4.115 -- AgentMove side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_MOVEMsgParser. The data plane (classify_move) decides which action to take;
// this header captures the ordered side-effect list the orchestrator must execute.
//
// Legacy branches:
//   - user not in map: drop_no_map (silent drop).
//   - user in map: forward_to_map (TransToMapServerMsgParser raw forward).
//
// Side effects:
//   - forward_to_map: raw forward MSG_DWORD to user->dwMapServerConnectionIndex.
//   - drop_no_map: silent drop (no side effect).
//

#include <cstdint>
#include <vector>

#include "mxh/server/agent_move.hpp"

namespace mxh::server {

// Side-effect kinds the orchestrator must dispatch in order.
enum class MoveSideEffectKind : std::uint8_t {
    Drop,                                  // user not in map
    ForwardRawToMap,                       // raw forward MSG_DWORD to user->dwMapServerConnectionIndex
};

struct MoveSideEffect final {
    MoveSideEffectKind kind = MoveSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t object_id = 0u;
};

struct MoveSideEffectPlan final {
    std::vector<MoveSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool move_effect_targets_map(const MoveSideEffect& e) noexcept {
    return e.kind == MoveSideEffectKind::ForwardRawToMap;
}

inline MoveSideEffectPlan move_side_effect_plan(const MoveAction& a) {
    MoveSideEffectPlan plan;
    using K = MoveSideEffectKind;
    using A = MoveActionKind;
    switch (a.kind) {
        case A::drop_no_map:
            plan.drop = true;
            plan.effects.push_back({K::Drop, a.protocol, a.object_id});
            return plan;
        case A::forward_to_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToMap, a.protocol, a.object_id});
            return plan;
        case A::forward_to_map_if_in_map:
            // legacy classifier does not emit this kind; treat identically to forward_to_map
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToMap, a.protocol, a.object_id});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server