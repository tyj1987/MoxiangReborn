//
// CItemManager::MP_ITEM_SHOPITEM_RARECREATE_SYN from legacy
// [Server]Map/ItemManager.cpp:5924-6029.
//
// The legacy handler runs 11 gates on a MSG_DWORD4 with NACK codes
// 1..11 (no RARECREATE_ACK is sent on success; only USE_ACK):
//   1. IsUseAbleShopItem - code 1.
//   2. pShopItem + pTargetItem (GetItemInfoAbsIn x2) - code 2.
//   3. pShopItemInfo + pItemInfo (GetItemInfo x2) - code 3.
//   4. pShopItem->wIconIdx in {eSundries_RareItemCreate50, _70,
//      _90, _99} - code 4.
//   5. pItemInfo->ItemKind & eEQUIP_ITEM - code 5.
//   6. pTargetItem->Durability != 0 or pOptionInfo->dwOptionIdx != 0
//      (target already has an option) - code 6.
//   7. pItemInfo->ItemKind <= eEQUIP_ITEM_SHOES and
//      pTargetItem->wIconIdx % 10 != 0 (suffixed item) - code 7.
//   8. shopItemInfo.GenGol <= itemInfo.LimitLevel <=
//      shopItemInfo.MinChub - code 8.
//   9. RAREITEMMGR->IsRareItemAble(target.wIconIdx) - code 9.
//  10. RAREITEMMGR->GetRare(target.wIconIdx, &rare, pPlayer, TRUE)
//      returned FALSE - code 10.
//  11. DiscardItem returned EI_TRUE - code 11.
//
// On success:
//   - ShopItemRareInsertToDB(player, target.wIconIdx, target.Position,
//     target.dwDBIdx, &rareoption).
//   - LogItemMoney (eLog_ShopItemUse).
//   - Send USE_ACK (SEND_SHOPITEM_BASEINFO {USE_ACK, ShopItemPos,
//     ShopItemIdx}).
//   - Note: NO RARECREATE_ACK is sent.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/CommonGameDefine.h eSundries_RareItemCreate*
inline constexpr std::uint32_t LEGACY_ESUNDRIES_RARE_CREATE_50 = 50u;
inline constexpr std::uint32_t LEGACY_ESUNDRIES_RARE_CREATE_70 = 51u;
inline constexpr std::uint32_t LEGACY_ESUNDRIES_RARE_CREATE_90 = 52u;
inline constexpr std::uint32_t LEGACY_ESUNDRIES_RARE_CREATE_99 = 53u;

enum class RareCreateOutcome : std::uint8_t {
    Success          = 0,
    NotUsable        = 1,
    BadItem          = 2,
    ItemInfoMissing  = 3,
    WrongIcon        = 4,
    NotEquip         = 5,
    AlreadyRare      = 6,
    WrongIconSuffix  = 7,
    LevelOutOfRange  = 8,
    NotRareAble      = 9,
    GetRareFailed    = 10,
    DiscardFailed    = 11,
};

struct RareCreateValidationInput final {
    bool shop_item_is_useable = false;
    bool shop_item_exists = false;
    bool target_item_exists = false;
    bool shop_item_info_exists = false;
    bool target_item_info_exists = false;
    bool shop_item_icon_is_create_50_70_90_99 = false;
    bool target_is_equip_kind = false;
    bool target_durability_zero = false;
    bool target_option_idx_zero = false;
    bool target_w_icon_idx_suffix_zero = false;
    bool level_in_range = false;     // shopItemInfo.GenGol <= itemInfo.LimitLevel <= shopItemInfo.MinChub
    bool is_rare_item_able = false;  // RAREITEMMGR->IsRareItemAble
    bool get_rare_returned_true = false;  // RAREITEMMGR->GetRare
    bool discard_returned_true = false;
};

inline bool is_rare_create_sundries_icon(std::uint32_t icon) noexcept {
    return icon == LEGACY_ESUNDRIES_RARE_CREATE_50 ||
           icon == LEGACY_ESUNDRIES_RARE_CREATE_70 ||
           icon == LEGACY_ESUNDRIES_RARE_CREATE_90 ||
           icon == LEGACY_ESUNDRIES_RARE_CREATE_99;
}

inline RareCreateOutcome classify_rare_create_outcome(
    const RareCreateValidationInput& in) noexcept {
    if (!in.shop_item_is_useable) return RareCreateOutcome::NotUsable;
    if (!in.shop_item_exists || !in.target_item_exists)
        return RareCreateOutcome::BadItem;
    if (!in.shop_item_info_exists || !in.target_item_info_exists)
        return RareCreateOutcome::ItemInfoMissing;
    if (!in.shop_item_icon_is_create_50_70_90_99)
        return RareCreateOutcome::WrongIcon;
    if (!in.target_is_equip_kind)
        return RareCreateOutcome::NotEquip;
    if (!in.target_durability_zero || !in.target_option_idx_zero)
        return RareCreateOutcome::AlreadyRare;
    if (!in.target_w_icon_idx_suffix_zero)
        return RareCreateOutcome::WrongIconSuffix;
    if (!in.level_in_range)
        return RareCreateOutcome::LevelOutOfRange;
    if (!in.is_rare_item_able)
        return RareCreateOutcome::NotRareAble;
    if (!in.get_rare_returned_true)
        return RareCreateOutcome::GetRareFailed;
    if (!in.discard_returned_true)
        return RareCreateOutcome::DiscardFailed;
    return RareCreateOutcome::Success;
}

inline std::uint8_t rare_create_nack_code(
    RareCreateOutcome o) noexcept {
    switch (o) {
        case RareCreateOutcome::NotUsable:       return 1u;
        case RareCreateOutcome::BadItem:         return 2u;
        case RareCreateOutcome::ItemInfoMissing: return 3u;
        case RareCreateOutcome::WrongIcon:       return 4u;
        case RareCreateOutcome::NotEquip:        return 5u;
        case RareCreateOutcome::AlreadyRare:     return 6u;
        case RareCreateOutcome::WrongIconSuffix: return 7u;
        case RareCreateOutcome::LevelOutOfRange: return 8u;
        case RareCreateOutcome::NotRareAble:     return 9u;
        case RareCreateOutcome::GetRareFailed:   return 10u;
        case RareCreateOutcome::DiscardFailed:   return 11u;
        default:                                 return 0u;
    }
}

enum class RareCreateSideEffectKind : std::uint8_t {
    SendNackToPlayer    = 0,
    SendUseAckToPlayer  = 1,  // legacy SEND_SHOPITEM_BASEINFO USE_ACK
    GenerateRareOption  = 2,  // legacy RAREITEMMGR->GetRare
    DiscardShopItem     = 3,
    ShopItemRareInsertToDB = 4, // legacy DB call
    LogItemMoney        = 5,
};

struct RareCreateSideEffect final {
    RareCreateSideEffectKind kind =
        RareCreateSideEffectKind::SendNackToPlayer;
    std::uint32_t player_id = 0;
    std::uint32_t nack_code = 0;
    std::uint16_t shop_item_idx = 0;
    std::uint16_t shop_item_pos = 0;
    std::uint32_t target_w_icon_idx = 0;
    std::uint32_t target_position = 0;
    std::uint32_t target_db_idx = 0;
};

struct RareCreateSideEffectPlan final {
    std::vector<RareCreateSideEffect> effects;
    bool send_nack = false;
    bool send_use_ack = false;
    bool generate_rare_option = false;
    bool discard_shop_item = false;
    bool db_rare_insert = false;
    bool log_item_money = false;
};

inline RareCreateSideEffectPlan rare_create_side_effect_plan(
    const RareCreateValidationInput& in,
    std::uint32_t player_id,
    std::uint16_t shop_item_idx,
    std::uint16_t shop_item_pos,
    std::uint32_t target_w_icon_idx,
    std::uint32_t target_position,
    std::uint32_t target_db_idx) {
    RareCreateSideEffectPlan plan;
    const RareCreateOutcome outcome = classify_rare_create_outcome(in);

    if (outcome != RareCreateOutcome::Success) {
        plan.send_nack = true;
        plan.effects.reserve(1u);
        RareCreateSideEffect nack{};
        nack.kind = RareCreateSideEffectKind::SendNackToPlayer;
        nack.player_id = player_id;
        nack.nack_code = rare_create_nack_code(outcome);
        plan.effects.push_back(nack);
        return plan;
    }

    plan.send_use_ack = true;
    plan.discard_shop_item = true;
    plan.db_rare_insert = true;
    plan.log_item_money = true;
    plan.generate_rare_option = true;

    plan.effects.reserve(5u);
    RareCreateSideEffect gen{};
    gen.kind = RareCreateSideEffectKind::GenerateRareOption;
    gen.player_id = player_id;
    gen.target_w_icon_idx = target_w_icon_idx;
    plan.effects.push_back(gen);
    RareCreateSideEffect discard{};
    discard.kind = RareCreateSideEffectKind::DiscardShopItem;
    discard.player_id = player_id;
    discard.shop_item_idx = shop_item_idx;
    discard.shop_item_pos = shop_item_pos;
    plan.effects.push_back(discard);
    RareCreateSideEffect db{};
    db.kind = RareCreateSideEffectKind::ShopItemRareInsertToDB;
    db.player_id = player_id;
    db.target_w_icon_idx = target_w_icon_idx;
    db.target_position = target_position;
    db.target_db_idx = target_db_idx;
    plan.effects.push_back(db);
    RareCreateSideEffect log{};
    log.kind = RareCreateSideEffectKind::LogItemMoney;
    log.player_id = player_id;
    plan.effects.push_back(log);
    RareCreateSideEffect use_ack{};
    use_ack.kind = RareCreateSideEffectKind::SendUseAckToPlayer;
    use_ack.player_id = player_id;
    use_ack.shop_item_idx = shop_item_idx;
    use_ack.shop_item_pos = shop_item_pos;
    plan.effects.push_back(use_ack);
    return plan;
}

}  // namespace mxh::server
