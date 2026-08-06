// 1:1 side-effect-dispatcher port of CItemManager's
// MP_ITEM_SHOPITEM_UNSEAL_SYN handler from legacy
// [Server]Map/ItemManager.cpp:5064-5085.
//
// The legacy handler calls ItemUnsealing(player, pos) and routes to
// one of two branches:
//   1. true (success): mutate msg.Protocol to MP_ITEM_SHOPITEM_UNSEAL_ACK
//      and SendMsg.
//   2. false (failure): mutate msg.Protocol to MP_ITEM_SHOPITEM_UNSEAL_NACK
//      and SendMsg.
//
// The msg.dwData is preserved across the protocol flip (legacy:
// msg.dwData = pmsg->dwData, both branches).

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_SHOPITEM_UNSEAL_ACK /
// _NACK. The Category is MP_ITEM.
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_UNSEAL_ACK  = 68u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_UNSEAL_NACK = 69u;

enum class ItemUnsealOutcome : std::uint8_t {
    Success = 0,
    Failure = 1,
};

inline ItemUnsealOutcome classify_item_unseal_outcome(
    bool unseal_ok) noexcept {
    if (unseal_ok) {
        return ItemUnsealOutcome::Success;
    }
    return ItemUnsealOutcome::Failure;
}

enum class ItemUnsealSideEffectKind : std::uint8_t {
    BroadcastUnsealAck  = 0,
    BroadcastUnsealNack = 1,
};

struct ItemUnsealSideEffect final {
    ItemUnsealSideEffectKind kind =
        ItemUnsealSideEffectKind::BroadcastUnsealAck;
    std::uint32_t dw_data = 0;       // legacy msg.dwData (= pmsg->dwData)
    std::uint16_t target_pos = 0;    // legacy pmsg->dwData (POSTYPE)
};

struct ItemUnsealSideEffectPlan final {
    std::vector<ItemUnsealSideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
};

inline ItemUnsealSideEffectPlan item_unseal_side_effect_plan(
    bool unseal_ok, std::uint32_t dw_data) {
    ItemUnsealSideEffectPlan plan;
    const ItemUnsealOutcome outcome = classify_item_unseal_outcome(unseal_ok);
    plan.effects.reserve(1u);
    ItemUnsealSideEffect eff{};
    eff.dw_data = dw_data;
    eff.target_pos = static_cast<std::uint16_t>(dw_data);
    if (outcome == ItemUnsealOutcome::Success) {
        plan.send_ack = true;
        eff.kind = ItemUnsealSideEffectKind::BroadcastUnsealAck;
    } else {
        plan.send_nack = true;
        eff.kind = ItemUnsealSideEffectKind::BroadcastUnsealNack;
    }
    plan.effects.push_back(eff);
    return plan;
}

}  // namespace mxh::server
