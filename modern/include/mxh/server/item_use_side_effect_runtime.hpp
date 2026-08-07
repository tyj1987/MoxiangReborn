// item_use_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// item_use_side_effect_plan(). The data plane returns a single-step
// plan (either BroadcastUseAck or BroadcastUseNack based on the
// legacy use_rt code); this header walks the plan and dispatches the
// single entry to a virtual ItemUseSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::MP_ITEM_USE_SYN from
// [Server]Map/ItemManager.cpp:4241-4265):
//   - use_rt == 0: legacy echoes the original pmsg as MSG_ITEM_USE_ACK
//     (memcpy + Protocol flip). The runtime's BroadcastUseAck carries
//     the original (target_pos, item_idx, original_rt) so the
//     orchestrator can rebuild the wire bytes via SendAckMsg().
//   - use_rt != 0: legacy sends MSG_ITEM_ERROR with Protocol =
//     MP_ITEM_USE_NACK, ECode = rt. The runtime's BroadcastUseNack
//     carries the same payload so the orchestrator can rebuild it via
//     SendErrorMsg().
//
// Pattern mirrors check_end_time_side_effect_runtime.hpp (D4.35) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/item_use_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ItemUse side-effect chain.
class ItemUseSideEffectSink {
public:
    virtual ~ItemUseSideEffectSink() = default;

    // Legacy: SendAckMsg(MP_ITEM_USE_ACK) -- echoes pmsg with
    // Protocol flipped to the ACK code.
    virtual void broadcast_use_ack(std::uint16_t target_pos,
                                   std::uint16_t item_idx,
                                   int original_rt) = 0;

    // Legacy: SendErrorMsg(MP_ITEM_USE_NACK, ECode=rt) -- sends the
    // error code to the originating player.
    virtual void broadcast_use_nack(std::uint16_t target_pos,
                                    std::uint16_t item_idx,
                                    int original_rt) = 0;
};

struct ItemUseRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t acks_sent       = 0;
    std::size_t nacks_sent      = 0;
    bool ack_flag_consumed   = false;
    bool nack_flag_consumed  = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline ItemUseRuntimeOutcome apply_item_use_side_effects(
    const ItemUseSideEffectPlan& plan,
    ItemUseSideEffectSink& sink) {
    ItemUseRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ItemUseSideEffectKind::BroadcastUseAck:
            sink.broadcast_use_ack(
                effect.target_pos, effect.item_idx, effect.original_rt);
            ++out.acks_sent;
            ++out.effects_applied;
            break;
        case ItemUseSideEffectKind::BroadcastUseNack:
            sink.broadcast_use_nack(
                effect.target_pos, effect.item_idx, effect.original_rt);
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
