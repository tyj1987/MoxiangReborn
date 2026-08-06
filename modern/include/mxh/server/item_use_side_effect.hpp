// 1:1 side-effect-dispatcher port of CItemManager::MP_ITEM_USE_SYN
// from legacy [Server]Map/ItemManager.cpp:4241-4265.
//
// The legacy handler calls UseItem(player, TargetPos, ItemIdx) and
// routes to one of two branches:
//   1. rt == eItemUseSuccess (rt == 0): echo pmsg as MSG_ITEM_USE_ACK
//      (memcpy + Protocol flip).
//   2. rt != 0: send MSG_ITEM_ERROR with Protocol = MP_ITEM_USE_NACK,
//      ECode = rt.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_USE_ACK / _NACK.
inline constexpr std::uint8_t LEGACY_MP_ITEM_USE_ACK  = 58u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_USE_NACK = 59u;

enum class ItemUseOutcome : std::uint8_t {
    Success = 0,  // legacy rt == 0
    Failure = 1,  // legacy rt != 0
};

inline ItemUseOutcome classify_item_use_outcome(int use_rt) noexcept {
    if (use_rt == 0) {
        return ItemUseOutcome::Success;
    }
    return ItemUseOutcome::Failure;
}

enum class ItemUseSideEffectKind : std::uint8_t {
    BroadcastUseAck  = 0,   // legacy SendAckMsg(MP_ITEM_USE_ACK)
    BroadcastUseNack = 1,   // legacy SendErrorMsg(MP_ITEM_USE_NACK, ECode)
};

struct ItemUseSideEffect final {
    ItemUseSideEffectKind kind =
        ItemUseSideEffectKind::BroadcastUseAck;
    std::uint16_t target_pos = 0;   // legacy pmsg->TargetPos
    std::uint16_t item_idx = 0;     // legacy pmsg->wItemIdx
    int original_rt = 0;
};

struct ItemUseSideEffectPlan final {
    std::vector<ItemUseSideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
};

inline ItemUseSideEffectPlan item_use_side_effect_plan(
    int use_rt,
    std::uint16_t target_pos,
    std::uint16_t item_idx) {
    ItemUseSideEffectPlan plan;
    const ItemUseOutcome outcome = classify_item_use_outcome(use_rt);
    if (outcome == ItemUseOutcome::Success) {
        plan.send_ack = true;
        plan.effects.reserve(1u);
        ItemUseSideEffect ack{};
        ack.kind = ItemUseSideEffectKind::BroadcastUseAck;
        ack.target_pos = target_pos;
        ack.item_idx = item_idx;
        ack.original_rt = use_rt;
        plan.effects.push_back(ack);
    } else {
        plan.send_nack = true;
        plan.effects.reserve(1u);
        ItemUseSideEffect nack{};
        nack.kind = ItemUseSideEffectKind::BroadcastUseNack;
        nack.target_pos = target_pos;
        nack.item_idx = item_idx;
        nack.original_rt = use_rt;
        plan.effects.push_back(nack);
    }
    return plan;
}

}  // namespace mxh::server
