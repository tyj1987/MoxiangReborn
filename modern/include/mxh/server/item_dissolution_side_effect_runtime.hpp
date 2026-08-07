// item_dissolution_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// item_dissolution_side_effect_plan(). The data plane returns a
// single-step plan (BroadcastDissolutionAck or
// BroadcastDissolutionNack based on the legacy ItemDissollution
// return code); this header walks the plan and dispatches the single
// entry to a virtual ItemDissolutionSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_DISSOLUTION_SYN from
// [Server]Map/ItemManager.cpp:4981-5001):
//   - ItemDissollution returns 0: legacy sends MSGBASE with
//     Protocol = MP_ITEM_DISSOLUTION_ACK (64).
//   - ItemDissollution returns non-zero: legacy sends MSGBASE with
//     Protocol = MP_ITEM_DISSOLUTION_NACK (65). Note: unlike other
//     item failures this is a plain NACK protocol, not MSG_ITEM_ERROR.
//
// Pattern mirrors item_combine_side_effect_runtime.hpp (D4.50) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/item_dissolution_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ItemDissolution side-effect chain.
class ItemDissolutionSideEffectSink {
public:
    virtual ~ItemDissolutionSideEffectSink() = default;

    // Legacy: SendMsgBase(MP_ITEM_DISSOLUTION_ACK) -- sends the
    // success protocol with the dissolution payload fields.
    virtual void broadcast_dissolution_ack(std::uint16_t item_idx,
                                           std::uint16_t item_pos,
                                           int original_rt) = 0;

    // Legacy: SendMsgBase(MP_ITEM_DISSOLUTION_NACK) -- sends the
    // plain NACK protocol (not MSG_ITEM_ERROR) on failure.
    virtual void broadcast_dissolution_nack(std::uint16_t item_idx,
                                            std::uint16_t item_pos,
                                            int original_rt) = 0;
};

struct ItemDissolutionRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t acks_sent       = 0;
    std::size_t nacks_sent      = 0;
    bool ack_flag_consumed   = false;
    bool nack_flag_consumed  = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline ItemDissolutionRuntimeOutcome apply_item_dissolution_side_effects(
    const ItemDissolutionSideEffectPlan& plan,
    ItemDissolutionSideEffectSink& sink) {
    ItemDissolutionRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ItemDissolutionSideEffectKind::BroadcastDissolutionAck:
            sink.broadcast_dissolution_ack(
                effect.item_idx, effect.item_pos, effect.original_rt);
            ++out.acks_sent;
            ++out.effects_applied;
            break;
        case ItemDissolutionSideEffectKind::BroadcastDissolutionNack:
            sink.broadcast_dissolution_nack(
                effect.item_idx, effect.item_pos, effect.original_rt);
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
