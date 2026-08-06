//
// CItemManager::MP_ITEM_SHOPITEM_REINFORCERESET_SYN from legacy
// [Server]Map/ItemManager.cpp:5833-5923.
//
// The legacy handler runs 7 gates on a MSG_DWORD4 with NACK codes
// 1/2/3/4/5/6/9 (codes 7 and 8 are legacy gaps):
//   1. IsUseAbleShopItem(pPlayer, dwData1, dwData2) - code 1.
//   2. pShopItem and pTargetItem both exist (GetItemInfoAbsIn x2) -
//      code 2.
//   3. pItemInfo = GetItemInfo(dwData3) - code 3.
//   4. pShopItem->wIconIdx == eIncantation_ReinforceReset - code 4.
//   5. pItemInfo->ItemKind & eEQUIP_ITEM and <= eEQUIP_ITEM_SHOES -
//      code 5.
//   6. pPlayer->GetItemOption(pTargetItem->Durability) != NULL -
//      code 6.
//   7. DiscardItem returns EI_TRUE - code 9.
//
// On success:
//   - LogItemMoney (eLog_ShopItemUse).
//   - pPlayer->RemoveItemOption(target.Durability).
//   - CharacterItemOptionDelete(target.Durability, target.dwDBIdx).
//   - ItemUpdateToDB(player, target.dwDBIdx, target.wIconIdx, 0,
//     target.Position, target.QuickPosition, target.RareIdx).
//   - LogItemMoney (eLog_ShopItem_ReinforceReset).
//   - target.Durability = 0.
//   - Send USE_ACK (SEND_SHOPITEM_BASEINFO {USE_ACK, ShopItemPos,
//     ShopItemIdx}).
//   - Send REINFORCERESET_ACK (MSG_DWORD).

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/CommonGameDefine.h eIncantation_ReinforceReset.
inline constexpr std::uint32_t LEGACY_EINCANTATION_REINFORCERESET = 40u;

enum class ReinforceResetOutcome : std::uint8_t {
    Success       = 0,
    NotUsable     = 1,  // NACK code = 1
    BadItem       = 2,  // NACK code = 2
    ItemInfoMissing = 3, // NACK code = 3
    WrongIcon     = 4,  // NACK code = 4 (not reinforce-reset)
    NotEquip      = 5,  // NACK code = 5
    NoOption      = 6,  // NACK code = 6
    DiscardFailed = 9,  // NACK code = 9 (legacy skip of 7/8)
};

struct ReinforceResetValidationInput final {
    bool shop_item_is_useable = false;
    bool shop_item_exists = false;
    bool target_item_exists = false;
    bool target_item_info_exists = false;
    bool shop_item_icon_is_reinforce_reset = false;
    bool target_is_equip_kind = false;
    bool target_has_option = false;
    bool discard_returned_true = false;
};

inline ReinforceResetOutcome classify_reinforce_reset_outcome(
    const ReinforceResetValidationInput& in) noexcept {
    if (!in.shop_item_is_useable) {
        return ReinforceResetOutcome::NotUsable;
    }
    if (!in.shop_item_exists || !in.target_item_exists) {
        return ReinforceResetOutcome::BadItem;
    }
    if (!in.target_item_info_exists) {
        return ReinforceResetOutcome::ItemInfoMissing;
    }
    if (!in.shop_item_icon_is_reinforce_reset) {
        return ReinforceResetOutcome::WrongIcon;
    }
    if (!in.target_is_equip_kind) {
        return ReinforceResetOutcome::NotEquip;
    }
    if (!in.target_has_option) {
        return ReinforceResetOutcome::NoOption;
    }
    if (!in.discard_returned_true) {
        return ReinforceResetOutcome::DiscardFailed;
    }
    return ReinforceResetOutcome::Success;
}

inline std::uint8_t reinforce_reset_nack_code(
    ReinforceResetOutcome o) noexcept {
    switch (o) {
        case ReinforceResetOutcome::NotUsable:       return 1u;
        case ReinforceResetOutcome::BadItem:         return 2u;
        case ReinforceResetOutcome::ItemInfoMissing: return 3u;
        case ReinforceResetOutcome::WrongIcon:       return 4u;
        case ReinforceResetOutcome::NotEquip:        return 5u;
        case ReinforceResetOutcome::NoOption:        return 6u;
        case ReinforceResetOutcome::DiscardFailed:   return 9u;
        default:                                     return 0u;
    }
}

enum class ReinforceResetSideEffectKind : std::uint8_t {
    SendAckToPlayer            = 0,
    SendNackToPlayer           = 1,
    SendUseAckToPlayer         = 2,  // SEND_SHOPITEM_BASEINFO USE_ACK
    DiscardShopItem            = 3,
    RemoveItemOption           = 4,
    CharacterItemOptionDelete  = 5,  // DB
    ItemUpdateToDB             = 6,  // DB
    LogItemMoneyUse            = 7,
    LogItemMoneyReset          = 8,
    ClearTargetDurability      = 9,
};

struct ReinforceResetSideEffect final {
    ReinforceResetSideEffectKind kind =
        ReinforceResetSideEffectKind::SendAckToPlayer;
    std::uint32_t player_id = 0;
    std::uint32_t nack_code = 0;
    std::uint16_t shop_item_idx = 0;
    std::uint16_t shop_item_pos = 0;
    std::uint32_t target_db_idx = 0;
    std::uint32_t target_durability = 0;
};

struct ReinforceResetSideEffectPlan final {
    std::vector<ReinforceResetSideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
    bool send_use_ack = false;
    bool discard_shop_item = false;
    bool remove_item_option = false;
    bool db_item_option_delete = false;
    bool db_item_update = false;
    bool log_item_money_use = false;
    bool log_item_money_reset = false;
    bool clear_target_durability = false;
};

inline ReinforceResetSideEffectPlan reinforce_reset_side_effect_plan(
    const ReinforceResetValidationInput& in,
    std::uint32_t player_id,
    std::uint16_t shop_item_idx,
    std::uint16_t shop_item_pos,
    std::uint32_t target_db_idx,
    std::uint32_t target_durability) {
    ReinforceResetSideEffectPlan plan;
    const ReinforceResetOutcome outcome =
        classify_reinforce_reset_outcome(in);

    if (outcome != ReinforceResetOutcome::Success) {
        plan.send_nack = true;
        plan.effects.reserve(1u);
        ReinforceResetSideEffect nack{};
        nack.kind = ReinforceResetSideEffectKind::SendNackToPlayer;
        nack.player_id = player_id;
        nack.nack_code = reinforce_reset_nack_code(outcome);
        plan.effects.push_back(nack);
        return plan;
    }

    plan.send_ack = true;
    plan.send_use_ack = true;
    plan.discard_shop_item = true;
    plan.remove_item_option = true;
    plan.db_item_option_delete = true;
    plan.db_item_update = true;
    plan.log_item_money_use = true;
    plan.log_item_money_reset = true;
    plan.clear_target_durability = true;

    plan.effects.reserve(9u);
    ReinforceResetSideEffect discard{};
    discard.kind = ReinforceResetSideEffectKind::DiscardShopItem;
    discard.player_id = player_id;
    discard.shop_item_idx = shop_item_idx;
    discard.shop_item_pos = shop_item_pos;
    plan.effects.push_back(discard);
    ReinforceResetSideEffect log_use{};
    log_use.kind = ReinforceResetSideEffectKind::LogItemMoneyUse;
    log_use.player_id = player_id;
    plan.effects.push_back(log_use);
    ReinforceResetSideEffect remove{};
    remove.kind = ReinforceResetSideEffectKind::RemoveItemOption;
    remove.player_id = player_id;
    remove.target_db_idx = target_db_idx;
    remove.target_durability = target_durability;
    plan.effects.push_back(remove);
    ReinforceResetSideEffect dbo{};
    dbo.kind = ReinforceResetSideEffectKind::CharacterItemOptionDelete;
    dbo.player_id = player_id;
    dbo.target_db_idx = target_db_idx;
    dbo.target_durability = target_durability;
    plan.effects.push_back(dbo);
    ReinforceResetSideEffect item_upd{};
    item_upd.kind = ReinforceResetSideEffectKind::ItemUpdateToDB;
    item_upd.player_id = player_id;
    item_upd.target_db_idx = target_db_idx;
    plan.effects.push_back(item_upd);
    ReinforceResetSideEffect log_reset{};
    log_reset.kind = ReinforceResetSideEffectKind::LogItemMoneyReset;
    log_reset.player_id = player_id;
    plan.effects.push_back(log_reset);
    ReinforceResetSideEffect clear{};
    clear.kind = ReinforceResetSideEffectKind::ClearTargetDurability;
    clear.player_id = player_id;
    clear.target_db_idx = target_db_idx;
    plan.effects.push_back(clear);
    ReinforceResetSideEffect use_ack{};
    use_ack.kind = ReinforceResetSideEffectKind::SendUseAckToPlayer;
    use_ack.player_id = player_id;
    use_ack.shop_item_idx = shop_item_idx;
    use_ack.shop_item_pos = shop_item_pos;
    plan.effects.push_back(use_ack);
    ReinforceResetSideEffect ack{};
    ack.kind = ReinforceResetSideEffectKind::SendAckToPlayer;
    ack.player_id = player_id;
    plan.effects.push_back(ack);
    return plan;
}

}  // namespace mxh::server
