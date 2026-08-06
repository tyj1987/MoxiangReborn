
//
// D4.102 -- AgentWanted side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_WANTEDServerMsgParser (around line 2397). The data plane (classify_wanted)
// decides which action to take; this header captures the ordered side-effect
// list the orchestrator must execute.
//
// Legacy branches:
//   - notify_delete/regist/notcomplete/destroyed (8,9,18,28) -> Broadcast2MapServerExceptOne.
//   - notcomplete_to_agent (23) with user_found: rewrite Protocol to
//     NOTCOMPLETE_BY_DELCHR (24) and Send2Server(user->dwMapServerConnectionIndex,
//     sizeof(MSG_DWORD)). NOTE: legacy uses sizeof(MSG_DWORD) (legacy quirk).
//   - notcomplete_to_agent without user_found: return (drop).
//   - default: TransToClientMsgParser.

#include <cstdint>
#include <vector>

#include "mxh/server/agent_wanted.hpp"

namespace mxh::server {

// SERVER side-effect kinds the orchestrator must dispatch in order.
enum class WantedSideEffectKind : std::uint8_t {
    Drop,                                  // no_user or unknown
    BroadcastToOtherMaps,                  // raw Broadcast2MapServerExceptOne
    SendNotCompleteByDelChrToMap,           // rewrite Protocol to NOTCOMPLETE_BY_DELCHR, send MSG_DWORD to user->dwMapServerConnectionIndex
    ForwardRawToClient,                    // TransToClientMsgParser default
};

struct WantedSideEffect final {
    WantedSideEffectKind kind = WantedSideEffectKind::Drop;
    std::uint32_t object_id = 0u;
    std::uint8_t target_map_connection_index = 0u;
};

struct WantedSideEffectPlan final {
    std::vector<WantedSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool wanted_effect_targets_map(const WantedSideEffect& e) noexcept {
    return e.kind == WantedSideEffectKind::BroadcastToOtherMaps ||
           e.kind == WantedSideEffectKind::SendNotCompleteByDelChrToMap;
}

inline bool wanted_effect_targets_client(const WantedSideEffect& e) noexcept {
    return e.kind == WantedSideEffectKind::ForwardRawToClient;
}

inline WantedSideEffectPlan wanted_side_effect_plan(const WantedAction& action) {
    WantedSideEffectPlan plan;
    using K = WantedSideEffectKind;
    using A = WantedServerActionKind;
    switch (action.kind) {
        case A::drop_no_user:
            plan.drop = true;
            plan.effects.push_back({K::Drop, action.object_id, 0u});
            return plan;
        case A::broadcast_to_other_maps:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::BroadcastToOtherMaps, action.object_id, 0u});
            return plan;
        case A::complete_notcomplete_send_to_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendNotCompleteByDelChrToMap, action.object_id, action.target_map_connection_index});
            return plan;
        case A::default_forward_to_client:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToClient, action.object_id, 0u});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server