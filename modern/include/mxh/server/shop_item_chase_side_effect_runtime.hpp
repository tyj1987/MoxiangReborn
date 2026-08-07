// shop_item_chase_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// shop_item_chase_side_effect_plan(). The data plane returns an empty
// plan (non-chase kind), a single ForwardChaseNackToAgent entry
// (target offline), or the 2-step success chain (ForwardChaseAckToAgent
// -> BroadcastChaseTracking); this header walks the plan and
// dispatches each entry to a virtual ShopItemChaseSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_CHASE_SYN from
// [Server]Map/ItemManager.cpp:5502-5545):
//   - FindUser(dwData2) returns null: handler sends
//     MP_ITEM_SHOPITEM_CHASE_NACK (99) with dwData = pmsg->dwData1 to
//     the agent server.
//   - dwData3 is not a tracking variant ({eIncantation_Tracking=31,
//     _Tracking7=32, _Tracking7_NoTrade=33}): handler returns with no
//     effect (empty plan).
//   - Tracking variant: (1) SEND_CHASEINFO {MP_ITEM, CHASE_ACK(98),
//     player id, name, position, MapNum, EventMapNum (44 if suryun
//     battle else channel id), CharacterIdx=dwData1} to agent, then
//     (2) MSGBASE {MP_ITEM, CHASE_TRACKING(100)} to the player.
//
// Pattern mirrors avatar_change_side_effect_runtime.hpp (D4.70) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/shop_item_chase_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ShopItemChase side-effect chain.
class ShopItemChaseSideEffectSink {
public:
    virtual ~ShopItemChaseSideEffectSink() = default;

    // Legacy: Send2AgentServer(SEND_CHASEINFO, MP_ITEM,
    // MP_ITEM_SHOPITEM_CHASE_ACK) -- resolves the target location to
    // the agent.
    virtual void forward_chase_ack_to_agent(
        std::uint32_t target_id, std::uint32_t requester_char_idx,
        std::uint32_t item_kind, int map_num, int event_map_num) = 0;

    // Legacy: Send2AgentServer(MP_ITEM_SHOPITEM_CHASE_NACK) with
    // dwData = requester_char_idx (target offline).
    virtual void forward_chase_nack_to_agent(
        std::uint32_t requester_char_idx) = 0;

    // Legacy: SendMsg(MP_ITEM_SHOPITEM_CHASE_TRACKING) to the target
    // player.
    virtual void broadcast_chase_tracking(std::uint32_t target_id,
                                          std::uint32_t item_kind) = 0;
};

struct ShopItemChaseRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t acks_forwarded  = 0;
    std::size_t nacks_forwarded = 0;
    std::size_t trackings       = 0;
    bool ack_flag_consumed      = false;
    bool nack_flag_consumed     = false;
    bool tracking_flag_consumed = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline ShopItemChaseRuntimeOutcome apply_shop_item_chase_side_effects(
    const ShopItemChaseSideEffectPlan& plan,
    ShopItemChaseSideEffectSink& sink) {
    ShopItemChaseRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ShopItemChaseSideEffectKind::ForwardChaseAckToAgent:
            sink.forward_chase_ack_to_agent(
                effect.target_id, effect.requester_char_idx,
                effect.item_kind, effect.map_num, effect.event_map_num);
            ++out.acks_forwarded;
            ++out.effects_applied;
            break;
        case ShopItemChaseSideEffectKind::ForwardChaseNackToAgent:
            sink.forward_chase_nack_to_agent(
                effect.requester_char_idx);
            ++out.nacks_forwarded;
            ++out.effects_applied;
            break;
        case ShopItemChaseSideEffectKind::BroadcastChaseTracking:
            sink.broadcast_chase_tracking(effect.target_id,
                                          effect.item_kind);
            ++out.trackings;
            ++out.effects_applied;
            break;
        }
    }
    out.ack_flag_consumed = plan.forward_ack;
    out.nack_flag_consumed = plan.forward_nack;
    out.tracking_flag_consumed = plan.broadcast_tracking;
    return out;
}

}  // namespace mxh::server
