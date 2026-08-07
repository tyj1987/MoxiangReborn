// item_sell_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// item_sell_side_effect_plan(). The data plane returns a single-step
// plan (either BroadcastSellAck or BroadcastSellNack based on the
// legacy SellItem return code); this header walks the plan and
// dispatches the single entry to a virtual ItemSellSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::MP_ITEM_SELL_SYN from
// [Server]Map/ItemManager.cpp:4205-4240):
//   - CheckHackNpc fails: legacy sends MSG_ITEM_ERROR with Protocol =
//     MP_ITEM_SELL_NACK, ECode = NOT_EXIST (= 103), and does NOT call
//     SellItem. The runtime's BroadcastSellNack carries ecode = 103 and
//     original_rt = -1 (the "no SellItem call" sentinel).
//   - SellItem returns 0: legacy echoes the original pmsg as
//     MSG_ITEM_SELL_ACK (memcpy + Protocol flip). The runtime's
//     BroadcastSellAck carries (target_pos, item_idx, item_num,
//     dealer_idx, original_rt) so the orchestrator can rebuild the wire
//     bytes via SendAckMsg().
//   - SellItem returns non-zero: legacy sends MSG_ITEM_ERROR with
//     Protocol = MP_ITEM_SELL_NACK, ECode = rt. The runtime's
//     BroadcastSellNack carries the same payload so the orchestrator
//     can rebuild it via SendErrorMsg().
//
// Pattern mirrors item_use_side_effect_runtime.hpp (D4.48) and the
// rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/item_sell_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ItemSell side-effect chain.
class ItemSellSideEffectSink {
public:
    virtual ~ItemSellSideEffectSink() = default;

    // Legacy: SendAckMsg(MP_ITEM_SELL_ACK) -- echoes pmsg with
    // Protocol flipped to the ACK code.
    virtual void broadcast_sell_ack(std::uint16_t target_pos,
                                    std::uint16_t item_idx,
                                    std::uint16_t item_num,
                                    std::uint16_t dealer_idx,
                                    int original_rt) = 0;

    // Legacy: SendErrorMsg(MP_ITEM_SELL_NACK, ECode=ecode) -- sends
    // the error code to the originating player. original_rt == -1
    // means the NPC gate rejected the request before SellItem ran
    // (ecode is then LEGACY_NOT_EXIST).
    virtual void broadcast_sell_nack(std::uint16_t target_pos,
                                     std::uint16_t item_idx,
                                     std::uint16_t item_num,
                                     std::uint16_t dealer_idx,
                                     int original_rt,
                                     int ecode) = 0;
};

struct ItemSellRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t acks_sent       = 0;
    std::size_t nacks_sent      = 0;
    bool ack_flag_consumed   = false;
    bool nack_flag_consumed  = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline ItemSellRuntimeOutcome apply_item_sell_side_effects(
    const ItemSellSideEffectPlan& plan,
    ItemSellSideEffectSink& sink) {
    ItemSellRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ItemSellSideEffectKind::BroadcastSellAck:
            sink.broadcast_sell_ack(
                effect.target_pos, effect.item_idx, effect.item_num,
                effect.dealer_idx, effect.original_rt);
            ++out.acks_sent;
            ++out.effects_applied;
            break;
        case ItemSellSideEffectKind::BroadcastSellNack:
            sink.broadcast_sell_nack(
                effect.target_pos, effect.item_idx, effect.item_num,
                effect.dealer_idx, effect.original_rt, effect.ecode);
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
