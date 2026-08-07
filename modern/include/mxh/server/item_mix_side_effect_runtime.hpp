// item_mix_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// item_mix_side_effect_plan(). The data plane returns a single-step
// plan (one of five broadcast kinds based on the legacy MixItem
// return code); this header walks the plan and dispatches the entry
// to a virtual ItemMixSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::MP_ITEM_MIX_SYN from
// [Server]Map/ItemManager.cpp:4360-4424):
//   - rt == 0: echo pmsg as MSG_ITEM_MIX_ACK with Protocol =
//     MP_ITEM_MIX_SUCCESS_ACK (60).
//   - rt == 1000: echo pmsg with Protocol = MP_ITEM_MIX_BIGFAILED_ACK
//     (61).
//   - rt == 1001: echo pmsg with Protocol = MP_ITEM_MIX_FAILED_ACK
//     (62).
//   - rt in {20, 21, 22, 23}: send MSG_DWORD2 with Protocol =
//     MP_ITEM_MIX_MSG (63), dwData1 = rt, dwData2 = BasicItemPos.
//   - rt == 2 or other: ASSERT + send MSG_ITEM_ERROR with Protocol =
//     MP_ITEM_ERROR_NACK (99), ECode = rt.
//
// Pattern mirrors item_buy_side_effect_runtime.hpp (D4.51) and the
// rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/item_mix_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ItemMix side-effect chain.
class ItemMixSideEffectSink {
public:
    virtual ~ItemMixSideEffectSink() = default;

    // Legacy: SendAckMsg(MP_ITEM_MIX_SUCCESS_ACK) -- echo pmsg with
    // the protocol byte flipped to the success ACK.
    virtual void broadcast_success_ack(
        std::uint16_t basic_item_idx, std::uint16_t basic_item_pos,
        std::uint16_t result_index, std::uint16_t material_num,
        std::uint16_t shop_item_idx, std::uint16_t shop_item_pos,
        int original_rt) = 0;

    // Legacy: SendAckMsg(MP_ITEM_MIX_BIGFAILED_ACK).
    virtual void broadcast_big_fail_ack(
        std::uint16_t basic_item_idx, std::uint16_t basic_item_pos,
        std::uint16_t result_index, std::uint16_t material_num,
        std::uint16_t shop_item_idx, std::uint16_t shop_item_pos,
        int original_rt) = 0;

    // Legacy: SendAckMsg(MP_ITEM_MIX_FAILED_ACK).
    virtual void broadcast_fail_ack(
        std::uint16_t basic_item_idx, std::uint16_t basic_item_pos,
        std::uint16_t result_index, std::uint16_t material_num,
        std::uint16_t shop_item_idx, std::uint16_t shop_item_pos,
        int original_rt) = 0;

    // Legacy: SendMsgDword2ToPlayer(MP_ITEM_MIX_MSG, dwData1=rt,
    // dwData2=BasicItemPos).
    virtual void broadcast_mix_msg(std::uint16_t basic_item_pos,
                                   int original_rt) = 0;

    // Legacy: ASSERT + SendErrorMsg(MP_ITEM_ERROR_NACK, ECode=rt).
    virtual void broadcast_error_nack(
        std::uint16_t basic_item_idx, std::uint16_t basic_item_pos,
        std::uint16_t result_index, std::uint16_t material_num,
        std::uint16_t shop_item_idx, std::uint16_t shop_item_pos,
        int original_rt, int ecode) = 0;
};

struct ItemMixRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t success_acks    = 0;
    std::size_t big_fail_acks   = 0;
    std::size_t fail_acks       = 0;
    std::size_t msgs_sent       = 0;
    std::size_t error_nacks_sent = 0;
    bool ack_flag_consumed      = false;
    bool msg_flag_consumed      = false;
    bool error_nack_flag_consumed = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline ItemMixRuntimeOutcome apply_item_mix_side_effects(
    const ItemMixSideEffectPlan& plan,
    ItemMixSideEffectSink& sink) {
    ItemMixRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ItemMixSideEffectKind::BroadcastSuccessAck:
            sink.broadcast_success_ack(
                effect.basic_item_idx, effect.basic_item_pos,
                effect.result_index, effect.material_num,
                effect.shop_item_idx, effect.shop_item_pos,
                effect.original_rt);
            ++out.success_acks;
            ++out.effects_applied;
            break;
        case ItemMixSideEffectKind::BroadcastBigFailAck:
            sink.broadcast_big_fail_ack(
                effect.basic_item_idx, effect.basic_item_pos,
                effect.result_index, effect.material_num,
                effect.shop_item_idx, effect.shop_item_pos,
                effect.original_rt);
            ++out.big_fail_acks;
            ++out.effects_applied;
            break;
        case ItemMixSideEffectKind::BroadcastFailAck:
            sink.broadcast_fail_ack(
                effect.basic_item_idx, effect.basic_item_pos,
                effect.result_index, effect.material_num,
                effect.shop_item_idx, effect.shop_item_pos,
                effect.original_rt);
            ++out.fail_acks;
            ++out.effects_applied;
            break;
        case ItemMixSideEffectKind::BroadcastMixMsg:
            sink.broadcast_mix_msg(effect.basic_item_pos,
                                   effect.original_rt);
            ++out.msgs_sent;
            ++out.effects_applied;
            break;
        case ItemMixSideEffectKind::BroadcastErrorNack:
            sink.broadcast_error_nack(
                effect.basic_item_idx, effect.basic_item_pos,
                effect.result_index, effect.material_num,
                effect.shop_item_idx, effect.shop_item_pos,
                effect.original_rt, effect.ecode);
            ++out.error_nacks_sent;
            ++out.effects_applied;
            break;
        }
    }
    out.ack_flag_consumed = plan.send_ack;
    out.msg_flag_consumed = plan.send_msg;
    out.error_nack_flag_consumed = plan.send_error_nack;
    return out;
}

}  // namespace mxh::server
