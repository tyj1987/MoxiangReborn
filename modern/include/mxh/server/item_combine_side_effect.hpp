// 1:1 side-effect-dispatcher port of CItemManager::MP_ITEM_COMBINE_SYN
// from legacy [Server]Map/ItemManager.cpp:4068-4091.
//
// The legacy handler calls CombineItem(player, ItemIdx, FromPos, ToPos,
// FromDur, ToDur) and routes to one of two branches:
//   1. EI_TRUE (rt == 0): success -> echo pmsg as MSG_ITEM_COMBINE_ACK
//      (memcpy + Protocol flip).
//   2. rt != 0: failure -> send MSG_ITEM_ERROR with Protocol =
//      MP_ITEM_ERROR_NACK, ECode = eItemUseErr_Combine (= 3),
//      SendErrorMsg auxiliary = rt.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

inline constexpr int LEGACY_EITEMUSE_COMBINE = 3;
inline constexpr std::uint8_t LEGACY_MP_ITEM_COMBINE_ACK = 56u;

enum class ItemCombineOutcome : std::uint8_t {
    Success = 0,
    Failure = 1,
};

inline ItemCombineOutcome classify_item_combine_outcome(
    int combine_rt) noexcept {
    if (combine_rt == 0) {
        return ItemCombineOutcome::Success;
    }
    return ItemCombineOutcome::Failure;
}

enum class ItemCombineSideEffectKind : std::uint8_t {
    BroadcastCombineAck  = 0,
    BroadcastErrorNack   = 1,
};

struct ItemCombineSideEffect final {
    ItemCombineSideEffectKind kind =
        ItemCombineSideEffectKind::BroadcastCombineAck;
    std::uint16_t from_pos = 0;
    std::uint16_t to_pos = 0;
    std::uint16_t item_idx = 0;
    std::uint16_t from_dur = 0;
    std::uint16_t to_dur = 0;
    int original_rt = 0;
};

struct ItemCombineSideEffectPlan final {
    std::vector<ItemCombineSideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
};

inline ItemCombineSideEffectPlan item_combine_side_effect_plan(
    int combine_rt,
    std::uint16_t from_pos,
    std::uint16_t to_pos,
    std::uint16_t item_idx,
    std::uint16_t from_dur,
    std::uint16_t to_dur) {
    ItemCombineSideEffectPlan plan;
    const ItemCombineOutcome outcome =
        classify_item_combine_outcome(combine_rt);
    if (outcome == ItemCombineOutcome::Success) {
        plan.send_ack = true;
        plan.effects.reserve(1u);
        ItemCombineSideEffect ack{};
        ack.kind = ItemCombineSideEffectKind::BroadcastCombineAck;
        ack.from_pos = from_pos;
        ack.to_pos = to_pos;
        ack.item_idx = item_idx;
        ack.from_dur = from_dur;
        ack.to_dur = to_dur;
        ack.original_rt = combine_rt;
        plan.effects.push_back(ack);
    } else {
        plan.send_nack = true;
        plan.effects.reserve(1u);
        ItemCombineSideEffect nack{};
        nack.kind = ItemCombineSideEffectKind::BroadcastErrorNack;
        nack.from_pos = from_pos;
        nack.to_pos = to_pos;
        nack.item_idx = item_idx;
        nack.from_dur = from_dur;
        nack.to_dur = to_dur;
        nack.original_rt = combine_rt;
        plan.effects.push_back(nack);
    }
    return plan;
}

}  // namespace mxh::server
