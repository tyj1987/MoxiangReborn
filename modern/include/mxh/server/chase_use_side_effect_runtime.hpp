// chase_use_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// chase_use_side_effect_plan(). The data plane returns an empty plan
// (no player) or a single ACK/NACK entry; this header walks the plan
// and dispatches the entry to a virtual ChaseUseSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_CHASEUSE_SYN from
// [Server]Map/ItemManager.cpp:5485-5501):
//   - FindUser returns null: handler returns (empty plan).
//   - GetUsingItemInfo(wData1) != null (player has the chase item
//     equipped): handler sends MP_ITEM_SHOPITEM_CHASEUSE_ACK (86).
//   - GetUsingItemInfo(wData1) == null: handler sends
//     MP_ITEM_SHOPITEM_CHASEUSE_NACK (87).
//
// Pattern mirrors avatar_takeoff_side_effect_runtime.hpp (D4.69) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/chase_use_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ChaseUse side-effect chain.
class ChaseUseSideEffectSink {
public:
    virtual ~ChaseUseSideEffectSink() = default;

    // Legacy: SendMsg(MP_ITEM_SHOPITEM_CHASEUSE_ACK) -- the player has
    // the chase (location-track) item equipped.
    virtual void broadcast_chase_use_ack(std::uint16_t item_idx,
                                         std::uint16_t item_pos) = 0;

    // Legacy: SendMsg(MP_ITEM_SHOPITEM_CHASEUSE_NACK) -- the player
    // does not have the chase item equipped.
    virtual void broadcast_chase_use_nack(std::uint16_t item_idx,
                                          std::uint16_t item_pos) = 0;
};

struct ChaseUseRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t acks_sent       = 0;
    std::size_t nacks_sent      = 0;
    bool ack_flag_consumed   = false;
    bool nack_flag_consumed  = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline ChaseUseRuntimeOutcome apply_chase_use_side_effects(
    const ChaseUseSideEffectPlan& plan,
    ChaseUseSideEffectSink& sink) {
    ChaseUseRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ChaseUseSideEffectKind::BroadcastChaseUseAck:
            sink.broadcast_chase_use_ack(effect.item_idx,
                                         effect.item_pos);
            ++out.acks_sent;
            ++out.effects_applied;
            break;
        case ChaseUseSideEffectKind::BroadcastChaseUseNack:
            sink.broadcast_chase_use_nack(effect.item_idx,
                                          effect.item_pos);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        }
    }
    out.ack_flag_consumed = plan.send_ack;
    out.nack_flag_consumed = plan.send_nack;
    return out;
}

}  // namespace mxh::server
