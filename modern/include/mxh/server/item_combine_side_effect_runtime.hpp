// item_combine_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// item_combine_side_effect_plan(). The data plane returns a
// single-step plan (BroadcastCombineAck or BroadcastErrorNack based
// on the legacy CombineItem return code); this header walks the plan
// and dispatches the single entry to a virtual
// ItemCombineSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::MP_ITEM_COMBINE_SYN
// from [Server]Map/ItemManager.cpp:4068-4091):
//   - CombineItem returns 0: legacy echoes the original pmsg as
//     MSG_ITEM_COMBINE_ACK (memcpy + Protocol flip). The runtime's
//     BroadcastCombineAck carries (from_pos, to_pos, item_idx,
//     from_dur, to_dur, original_rt) so the orchestrator can rebuild
//     the wire bytes via SendAckMsg().
//   - CombineItem returns non-zero: legacy sends MSG_ITEM_ERROR with
//     Protocol = MP_ITEM_ERROR_NACK, ECode = eItemUseErr_Combine (= 3),
//     and the original rt as the SendErrorMsg auxiliary code. The
//     runtime's BroadcastErrorNack carries ecode = 3 + original_rt.
//
// Pattern mirrors item_divide_side_effect_runtime.hpp (D4.49) and the
// rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/item_combine_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ItemCombine side-effect chain.
class ItemCombineSideEffectSink {
public:
    virtual ~ItemCombineSideEffectSink() = default;

    // Legacy: SendAckMsg(MP_ITEM_COMBINE_ACK) -- echoes pmsg with
    // Protocol flipped to the ACK code.
    virtual void broadcast_combine_ack(std::uint16_t from_pos,
                                       std::uint16_t to_pos,
                                       std::uint16_t item_idx,
                                       std::uint16_t from_dur,
                                       std::uint16_t to_dur,
                                       int original_rt) = 0;

    // Legacy: SendErrorMsg(MP_ITEM_ERROR_NACK,
    // ECode=eItemUseErr_Combine, aux=rt) -- sends the fixed combine
    // error code plus the original rt as the auxiliary code.
    virtual void broadcast_error_nack(std::uint16_t from_pos,
                                      std::uint16_t to_pos,
                                      std::uint16_t item_idx,
                                      std::uint16_t from_dur,
                                      std::uint16_t to_dur,
                                      int original_rt,
                                      int ecode) = 0;
};

struct ItemCombineRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t acks_sent       = 0;
    std::size_t nacks_sent      = 0;
    bool ack_flag_consumed   = false;
    bool nack_flag_consumed  = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline ItemCombineRuntimeOutcome apply_item_combine_side_effects(
    const ItemCombineSideEffectPlan& plan,
    ItemCombineSideEffectSink& sink) {
    ItemCombineRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ItemCombineSideEffectKind::BroadcastCombineAck:
            sink.broadcast_combine_ack(
                effect.from_pos, effect.to_pos, effect.item_idx,
                effect.from_dur, effect.to_dur, effect.original_rt);
            ++out.acks_sent;
            ++out.effects_applied;
            break;
        case ItemCombineSideEffectKind::BroadcastErrorNack:
            sink.broadcast_error_nack(
                effect.from_pos, effect.to_pos, effect.item_idx,
                effect.from_dur, effect.to_dur,
                effect.original_rt, LEGACY_EITEMUSE_COMBINE);
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
