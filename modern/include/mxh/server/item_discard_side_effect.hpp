// 1:1 data-plane + side-effect-dispatcher port of CItemManager's
// MP_ITEM_DISCARD_SYN handler from legacy [Server]Map/ItemManager.cpp:4115-4161.
//
// The legacy handler routes to one of three branches:
//   1. Looted player (legacy LOOTINGMGR->IsLootedPlayer): rt = 10
//      -> send MP_ITEM_ERROR_NACK with ECode = eItemUseErr_Discard.
//   2. DiscardItem returns EI_TRUE (rt == 0): echo MSG_ITEM_DISCARD_ACK
//      (memcpy + Protocol flip) and emit LogItemMoney.
//   3. DiscardItem returns non-success (rt != 0): send
//      MP_ITEM_DISCARD_NACK with ECode = rt.
//
// The data plane below encodes the 3-way decision; the side-effect
// dispatcher emits the structured steps so the orchestrator can route
// each branch to its subsystem (BroadcastDiscardAck /
// BroadcastDiscardNack / LogItemMoney / SendErrorMsg).

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/game/item_types.hpp>

namespace mxh::server {

// 1:1 with legacy ItemSlot.h ERROR_ITEM enum (success = 0).
inline constexpr int LEGACY_EI_TRUE_DISCARD = 0;

// 1:1 with legacy LOOTINGMGR loot-state rt code used by the legacy
// handler (legacy: rt = 10 means "looted player cannot discard").
inline constexpr int LEGACY_DISCARD_RT_LOOTED = 10;

// 1:1 with legacy [CC]Header/CommonGameDefine.h eItemUse_Err.
inline constexpr int LEGACY_EITEMUSE_DISCARD = 5;

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_DISCARD_ACK /
// MP_ITEM_DISCARD_NACK / MP_ITEM_ERROR_NACK. The Category is MP_ITEM.
inline constexpr std::uint8_t LEGACY_MP_ITEM_DISCARD_ACK  = 52u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_DISCARD_NACK = 53u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_ERROR_NACK   = 99u;

// 1:1 with legacy [CC]Header/GameResourceManager.h eLog_ItemDiscard.
inline constexpr std::uint32_t LEGACY_ELOG_ITEM_DISCARD = 8u;

// 3-way decision from MP_ITEM_DISCARD_SYN.
enum class ItemDiscardOutcome : std::uint8_t {
    Success       = 0,  // legacy: DiscardItem returned 0
    Failure       = 1,  // legacy: DiscardItem returned non-zero
    LootedPlayer  = 2,  // legacy: IsLootedPlayer was true
};

// Pure decision function. The caller passes the legacy rt value
// returned by DiscardItem and the looted flag; the data plane
// produces the 3-way outcome.
inline ItemDiscardOutcome classify_item_discard_outcome(
    int discard_rt, bool is_looted) noexcept {
    if (is_looted) {
        return ItemDiscardOutcome::LootedPlayer;
    }
    if (discard_rt == LEGACY_EI_TRUE_DISCARD) {
        return ItemDiscardOutcome::Success;
    }
    return ItemDiscardOutcome::Failure;
}

// Side-effect kinds.
enum class ItemDiscardSideEffectKind : std::uint8_t {
    BroadcastDiscardAck = 0,    // legacy SendAckMsg(MP_ITEM_DISCARD_ACK)
    LogDiscardedItem = 1,       // legacy LogItemMoney(... eLog_ItemDiscard ...)
    BroadcastDiscardNack = 2,   // legacy SendErrorMsg(MP_ITEM_DISCARD_NACK, ECode=rt)
    BroadcastErrorNack = 3,     // legacy SendErrorMsg(MP_ITEM_ERROR_NACK, ECode=eItemUseErr_Discard)
};

struct ItemDiscardSideEffect final {
    ItemDiscardSideEffectKind kind =
        ItemDiscardSideEffectKind::BroadcastDiscardAck;
    std::uint16_t target_pos = 0;   // legacy pmsg->TargetPos
    std::uint16_t item_idx = 0;     // legacy pmsg->wItemIdx
    std::uint16_t item_num = 0;     // legacy pmsg->ItemNum
    int original_rt = 0;
    int ecode = 0;                  // legacy MSG_ITEM_ERROR::ECode
    std::uint32_t log_code = 0;
};

struct ItemDiscardSideEffectPlan final {
    std::vector<ItemDiscardSideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
    bool send_error_nack = false;
};

// 1:1 with legacy ItemManager::MP_ITEM_DISCARD_SYN. The success plan
// emits two steps in legacy order: ACK broadcast first (legacy memcpy
// flips Protocol to ACK), then LogItemMoney. The discard-failure plan
// emits a single DiscardNack broadcast. The looted-player plan emits
// a single ErrorNack broadcast with ECode = eItemUseErr_Discard.
inline ItemDiscardSideEffectPlan item_discard_side_effect_plan(
    int discard_rt,
    bool is_looted,
    std::uint16_t target_pos,
    std::uint16_t item_idx,
    std::uint16_t item_num) {
    ItemDiscardSideEffectPlan plan;
    const ItemDiscardOutcome outcome = classify_item_discard_outcome(
        discard_rt, is_looted);

    if (outcome == ItemDiscardOutcome::Success) {
        plan.send_ack = true;
        plan.effects.reserve(2u);

        ItemDiscardSideEffect ack{};
        ack.kind = ItemDiscardSideEffectKind::BroadcastDiscardAck;
        ack.target_pos = target_pos;
        ack.item_idx = item_idx;
        ack.item_num = item_num;
        ack.original_rt = discard_rt;
        plan.effects.push_back(ack);

        ItemDiscardSideEffect log{};
        log.kind = ItemDiscardSideEffectKind::LogDiscardedItem;
        log.target_pos = target_pos;
        log.item_idx = item_idx;
        log.item_num = item_num;
        log.original_rt = discard_rt;
        log.log_code = LEGACY_ELOG_ITEM_DISCARD;
        plan.effects.push_back(log);
    } else if (outcome == ItemDiscardOutcome::Failure) {
        plan.send_nack = true;
        plan.effects.reserve(1u);

        ItemDiscardSideEffect nack{};
        nack.kind = ItemDiscardSideEffectKind::BroadcastDiscardNack;
        nack.target_pos = target_pos;
        nack.item_idx = item_idx;
        nack.item_num = item_num;
        nack.original_rt = discard_rt;
        nack.ecode = discard_rt;
        plan.effects.push_back(nack);
    } else {
        // Looted player.
        plan.send_error_nack = true;
        plan.effects.reserve(1u);

        ItemDiscardSideEffect err{};
        err.kind = ItemDiscardSideEffectKind::BroadcastErrorNack;
        err.target_pos = target_pos;
        err.item_idx = item_idx;
        err.item_num = item_num;
        err.original_rt = LEGACY_DISCARD_RT_LOOTED;
        err.ecode = LEGACY_EITEMUSE_DISCARD;
        plan.effects.push_back(err);
    }
    return plan;
}

}  // namespace mxh::server



