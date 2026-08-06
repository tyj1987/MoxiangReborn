
//
// D4.118 -- AgentPacked side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_PACKEDMsgParser (lines 2074-2100). The data plane (classify_packed_user) decides
// which action to take; this header captures the ordered side-effect list the
// orchestrator must execute.
//
// Legacy branches:
//   - packed_normal (0): fanout_to_users (iterate receivers_present and Send2User each).
//   - packed_to_mapserver (1) with port found: send_to_map_server_by_port.
//   - packed_to_mapserver (1) without port: drop (default Action{}).
//   - packed_to_broad_mapserver (2): broadcast_to_other_maps (Broadcast2MapServerExceptOne).
//   - default: drop (unknown Action{}).
//
// Side effects:
//   - fanout_to_users: for each receiver in receivers_present, Send2User(connIdx, pMsg, data_size).
//   - send_to_map_server_by_port: Send2Server(target_map_port, pMsg, data_size).
//   - broadcast_to_other_maps: Broadcast2MapServerExceptOne.
//   - unknown: silent drop.
//

#include <cstdint>
#include <vector>

#include "mxh/server/agent_packed.hpp"

namespace mxh::server {

// Side-effect kinds the orchestrator must dispatch in order.
enum class PackedSideEffectKind : std::uint8_t {
    Drop,                                  // unknown or port not found
    FanoutToUsers,                         // Send2User each receiver in receivers_present
    SendToMapServerByPort,                 // Send2Server(target_map_port)
    BroadcastToOtherMaps,                  // Broadcast2MapServerExceptOne
};

struct PackedSideEffect final {
    PackedSideEffectKind kind = PackedSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t target_object_id = 0u;
    std::size_t receiver_count = 0u;
    std::uint16_t data_size = 0u;
};

struct PackedSideEffectPlan final {
    std::vector<PackedSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool packed_effect_targets_user(const PackedSideEffect& e) noexcept {
    return e.kind == PackedSideEffectKind::FanoutToUsers;
}

inline bool packed_effect_targets_map(const PackedSideEffect& e) noexcept {
    return e.kind == PackedSideEffectKind::SendToMapServerByPort ||
           e.kind == PackedSideEffectKind::BroadcastToOtherMaps;
}

inline PackedSideEffectPlan packed_side_effect_plan(const PackedAction& a) {
    PackedSideEffectPlan plan;
    using K = PackedSideEffectKind;
    using A = PackedActionKind;
    switch (a.kind) {
        case A::fanout_to_users:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::FanoutToUsers, a.protocol, a.target_object_id, a.receiver_count, a.data_size});
            return plan;
        case A::send_to_map_server_by_port:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendToMapServerByPort, a.protocol, a.target_object_id, a.receiver_count, a.data_size});
            return plan;
        case A::broadcast_to_other_maps:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::BroadcastToOtherMaps, a.protocol, a.target_object_id, a.receiver_count, a.data_size});
            return plan;
        case A::unknown:
            plan.drop = true;
            plan.effects.push_back({K::Drop, a.protocol, a.target_object_id, 0u, 0u});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server