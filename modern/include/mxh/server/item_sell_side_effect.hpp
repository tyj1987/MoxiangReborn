// 1:1 side-effect-dispatcher port of CItemManager::MP_ITEM_SELL_SYN
// from legacy [Server]Map/ItemManager.cpp:4205-4240.
//
// The legacy handler calls SellItem(player, TargetPos, SellItemIdx,
// SellItemNum, DealerIdx) and routes to one of two branches:
//   1. EI_TRUE (rt == 0): success -> echo pmsg as MSG_ITEM_SELL_ACK
//      (legacy memcpy + Protocol flip).
//   2. rt != 0: failure -> send MSG_ITEM_ERROR with Protocol =
//      MP_ITEM_SELL_NACK, ECode = rt (the SellItem return code).
//
// There is also an early-exit if CheckHackNpc fails; that branch
// always emits MP_ITEM_SELL_NACK with ECode = NOT_EXIST (= 103).
// The data plane encodes that gate as a third outcome that returns
// immediately (no SellItem call).

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/CommonGameDefine.h DEAL_BUY_ERROR enum.
inline constexpr int LEGACY_NOT_EXIST      = 103;
inline constexpr int LEGACY_NO_DEMANDITEM  = 108;

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_SELL_ACK / _NACK.
inline constexpr std::uint8_t LEGACY_MP_ITEM_SELL_ACK  = 54u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_SELL_NACK = 55u;

// 3-way decision.
enum class ItemSellOutcome : std::uint8_t {
    Success         = 0,  // legacy: SellItem returned 0
    Failure         = 1,  // legacy: SellItem returned non-zero
    NpcGateFailure  = 2,  // legacy: CheckHackNpc returned false (NPC proximity)
};

inline ItemSellOutcome classify_item_sell_outcome(
    int sell_rt, bool npc_gate_ok) noexcept {
    if (!npc_gate_ok) {
        return ItemSellOutcome::NpcGateFailure;
    }
    if (sell_rt == 0) {
        return ItemSellOutcome::Success;
    }
    return ItemSellOutcome::Failure;
}

enum class ItemSellSideEffectKind : std::uint8_t {
    BroadcastSellAck  = 0,   // legacy SendAckMsg(MP_ITEM_SELL_ACK)
    BroadcastSellNack = 1,   // legacy SendErrorMsg(MP_ITEM_SELL_NACK, ECode)
};

struct ItemSellSideEffect final {
    ItemSellSideEffectKind kind =
        ItemSellSideEffectKind::BroadcastSellAck;
    std::uint16_t target_pos = 0;
    std::uint16_t item_idx = 0;
    std::uint16_t item_num = 0;
    std::uint16_t dealer_idx = 0;
    int original_rt = 0;
    int ecode = 0;
};

struct ItemSellSideEffectPlan final {
    std::vector<ItemSellSideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
};

inline ItemSellSideEffectPlan item_sell_side_effect_plan(
    int sell_rt,
    bool npc_gate_ok,
    std::uint16_t target_pos,
    std::uint16_t item_idx,
    std::uint16_t item_num,
    std::uint16_t dealer_idx) {
    ItemSellSideEffectPlan plan;
    const ItemSellOutcome outcome =
        classify_item_sell_outcome(sell_rt, npc_gate_ok);

    if (outcome == ItemSellOutcome::Success) {
        plan.send_ack = true;
        plan.effects.reserve(1u);
        ItemSellSideEffect ack{};
        ack.kind = ItemSellSideEffectKind::BroadcastSellAck;
        ack.target_pos = target_pos;
        ack.item_idx = item_idx;
        ack.item_num = item_num;
        ack.dealer_idx = dealer_idx;
        ack.original_rt = sell_rt;
        plan.effects.push_back(ack);
    } else {
        plan.send_nack = true;
        plan.effects.reserve(1u);
        ItemSellSideEffect nack{};
        nack.kind = ItemSellSideEffectKind::BroadcastSellNack;
        nack.target_pos = target_pos;
        nack.item_idx = item_idx;
        nack.item_num = item_num;
        nack.dealer_idx = dealer_idx;
        if (outcome == ItemSellOutcome::NpcGateFailure) {
            nack.original_rt = -1;  // legacy: did not call SellItem
            nack.ecode = LEGACY_NOT_EXIST;
        } else {
            nack.original_rt = sell_rt;
            nack.ecode = sell_rt;
        }
        plan.effects.push_back(nack);
    }
    return plan;
}

}  // namespace mxh::server
