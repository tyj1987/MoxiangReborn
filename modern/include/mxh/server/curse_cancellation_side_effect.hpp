//
// CItemManager::MP_ITEMEXT_SHOPITEM_CURSE_CANCELLATION_SYN from legacy
// [Server]Map/ItemManager.cpp:6245-6333.
//
// The legacy handler runs 3 gates with NACK codes 1/2/3 (code 2
// covers both 'unique info missing' and 'unique item has no curse'):
//   1. CHKRT->ItemOf(pPlayer, wData1, dwData, 0, 0,
//      CB_EXIST|CB_ICONIDX) - code 1.
//   2. pInfo = GAMERESRCMNGR->GetUniqueItemOptionList(dwData) and
//      pInfo->dwCurseCancellation != 0 - code 2.
//   3. DiscardItem(pPlayer, wData2, pItem->wIconIdx, 1) == EI_TRUE -
//      code 3.
//
// On full success the handler:
//   a. Discards the shop item, sends SEND_SHOPITEM_BASEINFO {MP_ITEM,
//      USE_ACK, ShopItemPos, ShopItemIdx}.
//   b. Logs (eLog_ShopItemUse).
//   c. Discards the cursed item; if successful sends
//      MSG_ITEM_DISCARD_ACK {MP_ITEMEXT, CURSE_CANCELLATION_DELETEITEM,
//      TargetPos, wItemIdx, ItemNum}.
//   d. Logs (eLog_ItemDiscard).
//   e. Computes obtainItemNum = GetCanBuyNumInSpace(...) for the
//      restored unique item (pInfo->dwCurseCancellation count).
//   f. If obtainItemNum == 0: break (no ObtainItemEx call, no
//      CURSE_CANCELLATION_ACK).
//   g. Else: ObtainItemEx(pPlayer, ...) which sends CURSE_CANCELLATION_ACK.
//
// NACK is sent in all failure paths via the trailing
// UniqueItemCurseCancellation label.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

enum class CurseCancellationOutcome : std::uint8_t {
    FullCancel        = 0,  // legacy: success, ObtainItemEx sent ACK
    NoSpaceForRestore = 1,  // legacy: success path but no inventory room
    ItemNotExist      = 2,  // legacy NACK code = 1
    UniqueItemInvalid = 3,  // legacy NACK code = 2 (info missing OR not cursed)
    DiscardShopFailed = 4,  // legacy NACK code = 3
};

struct CurseCancellationValidationInput final {
    bool item_exists_at_target = false;          // gate 1 (CHKRT)
    bool unique_item_info_exists = false;        // gate 2a
    bool unique_item_is_cursed = false;          // gate 2b (dwCurseCancellation != 0)
    bool discard_shop_returned_true = false;     // gate 3
    bool obtain_space_available = false;         // obtainItemNum != 0
};

inline CurseCancellationOutcome classify_curse_cancellation_outcome(
    const CurseCancellationValidationInput& in) noexcept {
    if (!in.item_exists_at_target) {
        return CurseCancellationOutcome::ItemNotExist;
    }
    if (!in.unique_item_info_exists || !in.unique_item_is_cursed) {
        return CurseCancellationOutcome::UniqueItemInvalid;
    }
    if (!in.discard_shop_returned_true) {
        return CurseCancellationOutcome::DiscardShopFailed;
    }
    if (!in.obtain_space_available) {
        return CurseCancellationOutcome::NoSpaceForRestore;
    }
    return CurseCancellationOutcome::FullCancel;
}

inline std::uint8_t curse_cancellation_nack_code(
    CurseCancellationOutcome o) noexcept {
    switch (o) {
        case CurseCancellationOutcome::ItemNotExist:      return 1u;
        case CurseCancellationOutcome::UniqueItemInvalid: return 2u;
        case CurseCancellationOutcome::DiscardShopFailed: return 3u;
        default:                                          return 0u;
    }
}

enum class CurseCancellationSideEffectKind : std::uint8_t {
    SendNackToPlayer     = 0,
    SendUseAckToPlayer   = 1,  // SEND_SHOPITEM_BASEINFO USE_ACK
    DiscardShopItem      = 2,
    LogItemMoneyUse      = 3,
    DiscardCursedItem    = 4,
    SendDeleteItemAck    = 5,  // MSG_ITEM_DISCARD_ACK DELETEITEM
    LogItemMoneyDiscard  = 6,
    ObtainItemEx         = 7,  // legacy ObtainItemEx -> sends ACK
};

struct CurseCancellationSideEffect final {
    CurseCancellationSideEffectKind kind =
        CurseCancellationSideEffectKind::SendNackToPlayer;
    std::uint32_t player_id = 0;
    std::uint32_t nack_code = 0;
    std::uint16_t shop_item_idx = 0;
    std::uint16_t shop_item_pos = 0;
    std::uint32_t curse_cancellation_count = 0;  // pInfo->dwCurseCancellation
};

struct CurseCancellationSideEffectPlan final {
    std::vector<CurseCancellationSideEffect> effects;
    bool send_nack = false;
    bool send_use_ack = false;
    bool discard_shop = false;
    bool log_use = false;
    bool discard_cursed = false;
    bool send_delete_ack = false;
    bool log_discard = false;
    bool obtain_ex = false;
    std::uint8_t nack_code = 0;
};

inline CurseCancellationSideEffectPlan curse_cancellation_side_effect_plan(
    const CurseCancellationValidationInput& in,
    std::uint32_t player_id,
    std::uint16_t shop_item_idx,
    std::uint16_t shop_item_pos,
    std::uint32_t curse_cancellation_count) {
    CurseCancellationSideEffectPlan plan;
    const CurseCancellationOutcome outcome =
        classify_curse_cancellation_outcome(in);

    if (outcome == CurseCancellationOutcome::FullCancel) {
        plan.send_use_ack = true;
        plan.discard_shop = true;
        plan.log_use = true;
        plan.discard_cursed = true;
        plan.send_delete_ack = true;
        plan.log_discard = true;
        plan.obtain_ex = true;
        plan.effects.reserve(7u);
        CurseCancellationSideEffect discard_shop{};
        discard_shop.kind = CurseCancellationSideEffectKind::DiscardShopItem;
        discard_shop.player_id = player_id;
        discard_shop.shop_item_idx = shop_item_idx;
        discard_shop.shop_item_pos = shop_item_pos;
        plan.effects.push_back(discard_shop);
        CurseCancellationSideEffect log_use{};
        log_use.kind = CurseCancellationSideEffectKind::LogItemMoneyUse;
        log_use.player_id = player_id;
        plan.effects.push_back(log_use);
        CurseCancellationSideEffect use_ack{};
        use_ack.kind = CurseCancellationSideEffectKind::SendUseAckToPlayer;
        use_ack.player_id = player_id;
        use_ack.shop_item_idx = shop_item_idx;
        use_ack.shop_item_pos = shop_item_pos;
        plan.effects.push_back(use_ack);
        CurseCancellationSideEffect discard_cursed{};
        discard_cursed.kind =
            CurseCancellationSideEffectKind::DiscardCursedItem;
        discard_cursed.player_id = player_id;
        plan.effects.push_back(discard_cursed);
        CurseCancellationSideEffect delete_ack{};
        delete_ack.kind =
            CurseCancellationSideEffectKind::SendDeleteItemAck;
        delete_ack.player_id = player_id;
        plan.effects.push_back(delete_ack);
        CurseCancellationSideEffect log_disc{};
        log_disc.kind =
            CurseCancellationSideEffectKind::LogItemMoneyDiscard;
        log_disc.player_id = player_id;
        plan.effects.push_back(log_disc);
        CurseCancellationSideEffect obtain{};
        obtain.kind = CurseCancellationSideEffectKind::ObtainItemEx;
        obtain.player_id = player_id;
        obtain.curse_cancellation_count = curse_cancellation_count;
        plan.effects.push_back(obtain);
        return plan;
    }

    if (outcome == CurseCancellationOutcome::NoSpaceForRestore) {
        plan.send_use_ack = true;
        plan.discard_shop = true;
        plan.log_use = true;
        plan.discard_cursed = true;
        plan.send_delete_ack = true;
        plan.log_discard = true;
        plan.obtain_ex = false;
        plan.effects.reserve(6u);
        CurseCancellationSideEffect discard_shop{};
        discard_shop.kind = CurseCancellationSideEffectKind::DiscardShopItem;
        discard_shop.player_id = player_id;
        discard_shop.shop_item_idx = shop_item_idx;
        discard_shop.shop_item_pos = shop_item_pos;
        plan.effects.push_back(discard_shop);
        CurseCancellationSideEffect log_use{};
        log_use.kind = CurseCancellationSideEffectKind::LogItemMoneyUse;
        log_use.player_id = player_id;
        plan.effects.push_back(log_use);
        CurseCancellationSideEffect use_ack{};
        use_ack.kind = CurseCancellationSideEffectKind::SendUseAckToPlayer;
        use_ack.player_id = player_id;
        use_ack.shop_item_idx = shop_item_idx;
        use_ack.shop_item_pos = shop_item_pos;
        plan.effects.push_back(use_ack);
        CurseCancellationSideEffect discard_cursed{};
        discard_cursed.kind =
            CurseCancellationSideEffectKind::DiscardCursedItem;
        discard_cursed.player_id = player_id;
        plan.effects.push_back(discard_cursed);
        CurseCancellationSideEffect delete_ack{};
        delete_ack.kind =
            CurseCancellationSideEffectKind::SendDeleteItemAck;
        delete_ack.player_id = player_id;
        plan.effects.push_back(delete_ack);
        CurseCancellationSideEffect log_disc{};
        log_disc.kind =
            CurseCancellationSideEffectKind::LogItemMoneyDiscard;
        log_disc.player_id = player_id;
        plan.effects.push_back(log_disc);
        return plan;
    }

    plan.send_nack = true;
    plan.nack_code = curse_cancellation_nack_code(outcome);
    plan.effects.reserve(1u);
    CurseCancellationSideEffect nack{};
    nack.kind = CurseCancellationSideEffectKind::SendNackToPlayer;
    nack.player_id = player_id;
    nack.nack_code = plan.nack_code;
    plan.effects.push_back(nack);
    return plan;
}

}  // namespace mxh::server
