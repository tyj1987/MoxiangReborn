// 1:1 side-effect-dispatcher port of CItemManager::MP_ITEM_DISSOLUTION_SYN
// from legacy [Server]Map/ItemManager.cpp:4981-5001.
//
// The legacy handler calls DISSOLUTIONMGR->ItemDissollution and
// routes to one of two branches:
//   1. rt == 0: success -> send MSGBASE with
//      Protocol=MP_ITEM_DISSOLUTION_ACK.
//   2. rt != 0: failure -> send MSGBASE with
//      Protocol=MP_ITEM_DISSOLUTION_NACK.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

inline constexpr std::uint8_t LEGACY_MP_ITEM_DISSOLUTION_ACK  = 64u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_DISSOLUTION_NACK = 65u;

enum class ItemDissolutionOutcome : std::uint8_t {
    Success = 0,
    Failure = 1,
};

inline ItemDissolutionOutcome classify_item_dissolution_outcome(
    int dissolution_rt) noexcept {
    if (dissolution_rt == 0) {
        return ItemDissolutionOutcome::Success;
    }
    return ItemDissolutionOutcome::Failure;
}

enum class ItemDissolutionSideEffectKind : std::uint8_t {
    BroadcastDissolutionAck  = 0,
    BroadcastDissolutionNack = 1,
};

struct ItemDissolutionSideEffect final {
    ItemDissolutionSideEffectKind kind =
        ItemDissolutionSideEffectKind::BroadcastDissolutionAck;
    std::uint16_t item_idx = 0;     // legacy pmsg->ItemInfo.wIconIdx
    std::uint16_t item_pos = 0;     // legacy pmsg->ItemInfo.Position
    int original_rt = 0;
};

struct ItemDissolutionSideEffectPlan final {
    std::vector<ItemDissolutionSideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
};

inline ItemDissolutionSideEffectPlan item_dissolution_side_effect_plan(
    int dissolution_rt,
    std::uint16_t item_idx,
    std::uint16_t item_pos) {
    ItemDissolutionSideEffectPlan plan;
    const ItemDissolutionOutcome outcome =
        classify_item_dissolution_outcome(dissolution_rt);
    plan.effects.reserve(1u);
    ItemDissolutionSideEffect eff{};
    eff.item_idx = item_idx;
    eff.item_pos = item_pos;
    eff.original_rt = dissolution_rt;
    if (outcome == ItemDissolutionOutcome::Success) {
        plan.send_ack = true;
        eff.kind = ItemDissolutionSideEffectKind::BroadcastDissolutionAck;
    } else {
        plan.send_nack = true;
        eff.kind = ItemDissolutionSideEffectKind::BroadcastDissolutionNack;
    }
    plan.effects.push_back(eff);
    return plan;
}

}  // namespace mxh::server
