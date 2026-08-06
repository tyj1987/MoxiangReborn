// 1:1 side-effect-dispatcher port of CItemManager::MP_ITEM_BUY_SYN
// from legacy [Server]Map/ItemManager.cpp:4162-4204.
//
// The legacy handler runs three gates in order and routes to one of
// four branches:
//   1. CheckHackNpc fails: NACK with ECode = NOT_EXIST (= 103).
//   2. CheckDemandItem fails: NACK with ECode = NO_DEMANDITEM (= 108).
//   3. BuyItem success: silent (the BuyItem path emits its own
//      ITEMOBTAINARRAYINFO ACK through the ObtainItemEx flow).
//   4. BuyItem failure: NACK with ECode = rt.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/CommonGameDefine.h DEAL_BUY_ERROR enum.
inline constexpr int LEGACY_NOT_EXIST_BUY     = 103;
inline constexpr int LEGACY_NO_DEMANDITEM_BUY = 108;

inline constexpr std::uint8_t LEGACY_MP_ITEM_BUY_NACK = 51u;

enum class ItemBuyOutcome : std::uint8_t {
    Success         = 0,
    NpcGateFailure  = 1,
    DemandFailure   = 2,
    BuyFailure      = 3,
};

inline ItemBuyOutcome classify_item_buy_outcome(
    int buy_rt, bool npc_gate_ok, bool demand_ok) noexcept {
    if (!npc_gate_ok) {
        return ItemBuyOutcome::NpcGateFailure;
    }
    if (!demand_ok) {
        return ItemBuyOutcome::DemandFailure;
    }
    if (buy_rt == 0) {
        return ItemBuyOutcome::Success;
    }
    return ItemBuyOutcome::BuyFailure;
}

enum class ItemBuySideEffectKind : std::uint8_t {
    BroadcastNackNpcGate = 0,    // ECode = NOT_EXIST
    BroadcastNackDemand  = 1,    // ECode = NO_DEMANDITEM
    BroadcastNackBuyFail = 2,    // ECode = rt
};

struct ItemBuySideEffect final {
    ItemBuySideEffectKind kind =
        ItemBuySideEffectKind::BroadcastNackBuyFail;
    std::uint16_t buy_item_idx = 0;   // legacy pmsg->wBuyItemIdx
    std::uint16_t buy_item_num = 0;   // legacy pmsg->BuyItemNum
    std::uint16_t dealer_idx = 0;     // legacy pmsg->wDealerIdx
    int ecode = 0;
    int original_rt = 0;
};

struct ItemBuySideEffectPlan final {
    std::vector<ItemBuySideEffect> effects;
    bool send_nack = false;
    bool silent_success = false;
};

inline ItemBuySideEffectPlan item_buy_side_effect_plan(
    int buy_rt,
    bool npc_gate_ok,
    bool demand_ok,
    std::uint16_t buy_item_idx,
    std::uint16_t buy_item_num,
    std::uint16_t dealer_idx) {
    ItemBuySideEffectPlan plan;
    const ItemBuyOutcome outcome = classify_item_buy_outcome(
        buy_rt, npc_gate_ok, demand_ok);

    if (outcome == ItemBuyOutcome::Success) {
        plan.silent_success = true;
        return plan;
    }

    plan.send_nack = true;
    plan.effects.reserve(1u);
    ItemBuySideEffect nack{};
    nack.buy_item_idx = buy_item_idx;
    nack.buy_item_num = buy_item_num;
    nack.dealer_idx = dealer_idx;
    nack.original_rt = buy_rt;

    switch (outcome) {
    case ItemBuyOutcome::NpcGateFailure:
        nack.kind = ItemBuySideEffectKind::BroadcastNackNpcGate;
        nack.ecode = LEGACY_NOT_EXIST_BUY;
        nack.original_rt = -1;
        break;
    case ItemBuyOutcome::DemandFailure:
        nack.kind = ItemBuySideEffectKind::BroadcastNackDemand;
        nack.ecode = LEGACY_NO_DEMANDITEM_BUY;
        nack.original_rt = -1;
        break;
    case ItemBuyOutcome::BuyFailure:
        nack.kind = ItemBuySideEffectKind::BroadcastNackBuyFail;
        nack.ecode = buy_rt;
        nack.original_rt = buy_rt;
        break;
    default:
        break;
    }
    plan.effects.push_back(nack);
    return plan;
}

}  // namespace mxh::server
