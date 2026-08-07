// item_discard_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// item_discard_side_effect_plan(). The data plane returns an ordered
// effect list (2-step success chain, single discard-NACK, or single
// error-NACK for looted players); this header walks the plan and
// dispatches each entry to a virtual ItemDiscardSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::MP_ITEM_DISCARD_SYN
// from [Server]Map/ItemManager.cpp:4115-4161):
//   - Success (DiscardItem rt == 0, not looted): legacy echoes pmsg as
//     MSG_ITEM_DISCARD_ACK (memcpy + Protocol flip) FIRST, then emits
//     LogItemMoney(... eLog_ItemDiscard ...). The runtime applies the
//     two steps in that order.
//   - Failure (rt != 0, not looted): legacy sends
//     MSG_ITEM_DISCARD_NACK with ECode = rt. The runtime's
//     BroadcastDiscardNack carries ecode = original rt.
//   - Looted player (IsLootedPlayer true): legacy sends
//     MSG_ITEM_ERROR_NACK with ECode = eItemUseErr_Discard (= 5) and
//     the looted rt (= 10) as the auxiliary code. The runtime's
//     BroadcastErrorNack carries ecode = 5 + original_rt = 10.
//
// Pattern mirrors item_sell_side_effect_runtime.hpp (D4.47) and the
// rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/item_discard_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ItemDiscard side-effect chain.
class ItemDiscardSideEffectSink {
public:
    virtual ~ItemDiscardSideEffectSink() = default;

    // Legacy: SendAckMsg(MP_ITEM_DISCARD_ACK) -- echoes pmsg with
    // Protocol flipped to the ACK code.
    virtual void broadcast_discard_ack(std::uint16_t target_pos,
                                       std::uint16_t item_idx,
                                       std::uint16_t item_num,
                                       int original_rt) = 0;

    // Legacy: LogItemMoney(player, ..., eLog_ItemDiscard). The runtime
    // passes the payload fields plus the fixed log code from the data
    // plane so the caller can rebuild the money-log row.
    virtual void log_discarded_item(std::uint16_t target_pos,
                                    std::uint16_t item_idx,
                                    std::uint16_t item_num,
                                    std::uint32_t log_code) = 0;

    // Legacy: SendErrorMsg(MP_ITEM_DISCARD_NACK, ECode=rt) -- sends
    // the DiscardItem return code to the originating player.
    virtual void broadcast_discard_nack(std::uint16_t target_pos,
                                        std::uint16_t item_idx,
                                        std::uint16_t item_num,
                                        int original_rt,
                                        int ecode) = 0;

    // Legacy: SendErrorMsg(MP_ITEM_ERROR_NACK,
    // ECode=eItemUseErr_Discard, aux=looted_rt) -- looted players
    // cannot discard; ecode = eItemUseErr_Discard and original_rt is
    // the looted sentinel (= 10).
    virtual void broadcast_error_nack(std::uint16_t target_pos,
                                      std::uint16_t item_idx,
                                      std::uint16_t item_num,
                                      int original_rt,
                                      int ecode) = 0;
};

struct ItemDiscardRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t acks_sent       = 0;
    std::size_t logs_sent       = 0;
    std::size_t nacks_sent      = 0;
    std::size_t error_nacks_sent = 0;
    bool ack_flag_consumed      = false;
    bool nack_flag_consumed     = false;
    bool error_nack_flag_consumed = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline ItemDiscardRuntimeOutcome apply_item_discard_side_effects(
    const ItemDiscardSideEffectPlan& plan,
    ItemDiscardSideEffectSink& sink) {
    ItemDiscardRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ItemDiscardSideEffectKind::BroadcastDiscardAck:
            sink.broadcast_discard_ack(
                effect.target_pos, effect.item_idx, effect.item_num,
                effect.original_rt);
            ++out.acks_sent;
            ++out.effects_applied;
            break;
        case ItemDiscardSideEffectKind::LogDiscardedItem:
            sink.log_discarded_item(
                effect.target_pos, effect.item_idx, effect.item_num,
                effect.log_code);
            ++out.logs_sent;
            ++out.effects_applied;
            break;
        case ItemDiscardSideEffectKind::BroadcastDiscardNack:
            sink.broadcast_discard_nack(
                effect.target_pos, effect.item_idx, effect.item_num,
                effect.original_rt, effect.ecode);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        case ItemDiscardSideEffectKind::BroadcastErrorNack:
            sink.broadcast_error_nack(
                effect.target_pos, effect.item_idx, effect.item_num,
                effect.original_rt, effect.ecode);
            ++out.error_nacks_sent;
            ++out.effects_applied;
            break;
        }
    }
    out.ack_flag_consumed = plan.send_ack;
    out.nack_flag_consumed = plan.send_nack;
    out.error_nack_flag_consumed = plan.send_error_nack;
    return out;
}

}  // namespace mxh::server
