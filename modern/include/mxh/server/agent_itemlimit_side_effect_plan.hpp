
//
// D4.110 -- AgentItemLimit side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_ITEMLIMITServerMsgParser (lines 5211-5235). The data plane (classify_itemlimit)
// decides which action to take; this header captures the ordered side-effect
// list the orchestrator must execute.
//
// Legacy branches:
//   - addcount_to_map (0): raw Broadcast2MapServerExceptOne.
//   - default (incl. full_to_client (1)): TransToClientMsgParser fallback.
//

#include <cstdint>
#include <vector>

#include "mxh/server/agent_itemlimit.hpp"

namespace mxh::server {

// SERVER side-effect kinds the orchestrator must dispatch in order.
enum class ItemLimitSideEffectKind : std::uint8_t {
    BroadcastToOtherMaps,                  // raw Broadcast2MapServerExceptOne for ADDCOUNT_TO_MAP
    ForwardRawToClient,                    // TransToClientMsgParser fallback (incl. FULL_TO_CLIENT)
};

struct ItemLimitSideEffect final {
    ItemLimitSideEffectKind kind = ItemLimitSideEffectKind::ForwardRawToClient;
    std::uint8_t protocol = 0u;
};

struct ItemLimitSideEffectPlan final {
    std::vector<ItemLimitSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool itemlimit_effect_targets_map(const ItemLimitSideEffect& e) noexcept {
    return e.kind == ItemLimitSideEffectKind::BroadcastToOtherMaps;
}

inline bool itemlimit_effect_targets_client(const ItemLimitSideEffect& e) noexcept {
    return e.kind == ItemLimitSideEffectKind::ForwardRawToClient;
}

inline ItemLimitSideEffectPlan itemlimit_side_effect_plan(const ItemLimitAction& a) {
    ItemLimitSideEffectPlan plan;
    using K = ItemLimitSideEffectKind;
    using A = ItemLimitActionKind;
    switch (a.kind) {
        case A::broadcast_to_other_maps:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::BroadcastToOtherMaps, a.protocol});
            return plan;
        case A::forward_to_client:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToClient, a.protocol});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server