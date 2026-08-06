// 1:1 side-effect-dispatcher port of CShopItemManager::CheckEndTime from
// legacy [Server]Map/ShopItemManager.cpp. Splits the legacy function's
// row-level side-effect chain into:
//   1. Pure data plane (this header): given the expired indices from
//      collect_realtime_expired + the pre-fetched ShopItemBase row +
//      a player_id + dup_slot discriminator + env wrapper for ItemInfo
//      lookups, return the ordered side-effect list the legacy code
//      applies per row. The dispatcher captures the 4-step chain
//      (DiscardItemAttempt + BroadcastUseEnd + ShopItemDeleteToDB +
//      LogItemMoney) so the orchestrator can apply them without
//      re-reading the legacy body.
//   2. Orchestrator half (legacy): the function calls
//      ITEMMGR->DiscardItem + SendMsgDwordToPlayer +
//      ShopItemDeleteToDB + LogItemMoney in one step; the modern port
//      keeps the decision pure and lets the orchestrator route each
//      step to its respective subsystem.
//
// 1:1 invariants:
//   - The 4-step chain runs in the legacy order: discard attempt first
//     (legacy: failure ASSERTs and continues), broadcast, DB delete,
//     then log. The data plane emits them in the same order so the
//     orchestrator can apply them sequentially without reordering.
//   - The dup counter is bumped before the broadcast (legacy calls
//     it inline). The data plane reports the bump separately so the
//     caller can apply it via ShopItemManager::bump_dup_* helpers.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/dup_param.hpp>
#include <mxh/server/shop_item_manager.hpp>

namespace mxh::server {

// 1:1 with legacy MP_ITEM_SHOPITEM_USEEND protocol code. The category
// is MP_ITEM (legacy const), so only the protocol is needed at the
// data plane.
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_USEEND = 0u;

// Step kinds emitted by the side-effect dispatcher, in the order the
// legacy code applies them. Each step carries the data the orchestrator
// needs to forward to its respective subsystem.
enum class CheckEndTimeStepKind : std::uint8_t {
    DiscardItemAttempt = 0,  // legacy ITEMMGR->DiscardItem(player, Pos, wIconIdx, 1)
    BroadcastUseEnd    = 1,  // legacy SendMsgDwordToPlayer(MP_ITEM_SHOPITEM_USEEND)
    ShopItemDeleteToDB = 2,  // legacy ShopItemDeleteToDB(player_id, dwDBIdx)
    LogItemMoney       = 3,  // legacy LogItemMoney(player_id, name, 0, chr(34),
                             //           eLog_ShopItemUseEnd, ...)
    BumpDupCounter     = 4,  // legacy inline m_DupXxx++ via the dup_slot
};

struct CheckEndTimeStep final {
    CheckEndTimeStepKind kind = CheckEndTimeStepKind::DiscardItemAttempt;
    std::uint16_t w_icon_idx = 0;
    std::uint16_t item_pos   = 0;
    std::uint64_t db_idx     = 0;
    ShopItemDupSlot dup_slot = ShopItemDupSlot::None;
};

// 1:1 with legacy CheckEndTime's per-row side-effect chain. Returns
// the steps in the legacy order (DiscardItemAttempt, BumpDupCounter,
// BroadcastUseEnd, ShopItemDeleteToDB, LogItemMoney). The caller is
// expected to apply each step via its respective subsystem.
//
// Inputs:
//   - row               : the legacy SHOPITEMWITHTIME row bytes (legacy:
//                         passes pShopItem->ShopItem.ItemBase to all 4
//                         sites).
//   - player_id         : legacy m_pPlayer->GetID() used for the DB delete
//                         + log sites.
//   - dup_slot          : the legacy dup counter that was bumped when the
//                         item was first used; the data plane re-emits the
//                         bump so the orchestrator can decrement via
//                         ShopItemManager::bump_dup_*.
//
// The data plane never invokes the legacy hooks directly; it just
// records the sequence of calls the legacy code would have made.
inline std::vector<CheckEndTimeStep> check_end_time_side_effect(
    const game::ShopItemWithTime& row,
    std::uint32_t player_id,
    ShopItemDupSlot dup_slot) {
    (void)player_id;  // captured implicitly via the steps; the orchestrator
                       // reads player_id from the runtime player context.
    std::vector<CheckEndTimeStep> steps;
    steps.reserve(5u);

    // Step 1: legacy ITEMMGR->DiscardItem(player, Position, wIconIdx, 1).
    // On failure the legacy code ASSERTs; the data plane records the
    // attempt unconditionally so the orchestrator can decide how to
    // route the failure (skip remaining steps vs. continue).
    CheckEndTimeStep discard{};
    discard.kind = CheckEndTimeStepKind::DiscardItemAttempt;
    discard.item_pos = row.ShopItem.ItemBase.Position;
    discard.w_icon_idx = row.ShopItem.ItemBase.wIconIdx;
    discard.db_idx = row.ShopItem.ItemBase.dwDBIdx;
    steps.push_back(discard);

    // Step 2: legacy m_DupXxx bump (dup counter is reset to its
    // pre-use value when the item expires).
    if (dup_slot != ShopItemDupSlot::None) {
        CheckEndTimeStep dup{};
        dup.kind = CheckEndTimeStepKind::BumpDupCounter;
        dup.w_icon_idx = row.ShopItem.ItemBase.wIconIdx;
        dup.db_idx = row.ShopItem.ItemBase.dwDBIdx;
        dup.dup_slot = dup_slot;
        steps.push_back(dup);
    }

    // Step 3: legacy SendMsgDwordToPlayer(MP_ITEM_SHOPITEM_USEEND).
    CheckEndTimeStep broadcast{};
    broadcast.kind = CheckEndTimeStepKind::BroadcastUseEnd;
    broadcast.w_icon_idx = row.ShopItem.ItemBase.wIconIdx;
    broadcast.db_idx = row.ShopItem.ItemBase.dwDBIdx;
    steps.push_back(broadcast);

    // Step 4: legacy ShopItemDeleteToDB(player_id, dwDBIdx).
    CheckEndTimeStep db{};
    db.kind = CheckEndTimeStepKind::ShopItemDeleteToDB;
    db.w_icon_idx = row.ShopItem.ItemBase.wIconIdx;
    db.db_idx = row.ShopItem.ItemBase.dwDBIdx;
    steps.push_back(db);

    // Step 5: legacy LogItemMoney(player_id, ...). The log site uses
    // the same ItemBase + BeginTime/Remaintime fields, so the data
    // plane records the row bits and the orchestrator constructs the
    // full LogItemMoney call from the runtime player context.
    CheckEndTimeStep log{};
    log.kind = CheckEndTimeStepKind::LogItemMoney;
    log.w_icon_idx = row.ShopItem.ItemBase.wIconIdx;
    log.db_idx = row.ShopItem.ItemBase.dwDBIdx;
    steps.push_back(log);

    return steps;
}

}  // namespace mxh::server
