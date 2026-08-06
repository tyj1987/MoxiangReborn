// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_SHOPITEM_SEAL_SYN from legacy
// [Server]Map/ItemManager.cpp:5675-5775.
//
// The legacy handler seals a target item using a seal-card item. The
// flow runs 8 gates in order and routes to one of 9 branches (8 NACK
// error codes + 1 ACK success path):
//   1. FindUser (return if null).
//   2. !IsUseAbleShopItem(sealItem)  -> NACK code 1
//   3. !IsUseAbleShopItem(targetItem) -> NACK code 2
//   4. !pSealItem || !pTargetItem  -> NACK code 3
//   5. !pItemInfo  -> NACK code 3
//   6. pSealItem->wIconIdx != eIncantation_ItemSeal  -> NACK code 4
//   7. target kind not in {MAKEUP, DECORATION, PET}  -> NACK code 5
//   8. pItemInfo->SellPrice != eShopItemUseParam_Forever  -> NACK code 6
//   9. pTargetItem already sealed (ITEM_PARAM_SEAL)  -> NACK code 7
//  10. EI_TRUE != DiscardItem(seal)  -> NACK code 9
//  11. success path:
//      - LogItemMoney(eLog_ShopItemUse)
//      - Set ITEM_PARAM_SEAL on target + persist via
//        ShopItemParamUpdateToDB
//      - DeleteUsingShopItemInfo + ShopItemDeleteToDB
//      - LogItemMoney(eLog_ShopItemSeal)
//      - Send MP_ITEM_SHOPITEM_USE_ACK and MP_ITEM_SHOPITEM_SEAL_ACK

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_SHOPITEM_SEAL_ACK/_NACK.
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_SEAL_ACK  = 88u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_SEAL_NACK = 89u;

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_SHOPITEM_USE_ACK.
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_USE_ACK = 90u;

// 1:1 with legacy eIncantation_ItemSeal (the seal-card item kind).
inline constexpr std::uint16_t LEGACY_EINCANTATION_ITEM_SEAL = 39;

// 1:1 with legacy eSHOP_ITEM_* target kind whitelist.
inline constexpr std::uint8_t LEGACY_ESHOPITEM_MAKEUP     = 14;
inline constexpr std::uint8_t LEGACY_ESHOPITEM_DECORATION = 15;
inline constexpr std::uint8_t LEGACY_ESHOPITEM_PET        = 16;

// 1:1 with legacy eShopItemUseParam_Forever.
inline constexpr std::uint32_t LEGACY_ESHOPITEMUSEPARAM_FOREVER = 0xFFFFFFFBu;

// 1:1 with legacy ITEM_PARAM_SEAL bit.
inline constexpr std::uint32_t LEGACY_ITEM_PARAM_SEAL = 0x00000002u;

// 1:1 with legacy eItemUseSuccess (= 0) and the 8 NACK error codes.
inline constexpr std::uint32_t LEGACY_SEAL_NACK_NOT_USABLE_SEAL  = 1u;
inline constexpr std::uint32_t LEGACY_SEAL_NACK_NOT_USABLE_TARGET = 2u;
inline constexpr std::uint32_t LEGACY_SEAL_NACK_NOT_FOUND        = 3u;
inline constexpr std::uint32_t LEGACY_SEAL_NACK_WRONG_SEAL_ITEM  = 4u;
inline constexpr std::uint32_t LEGACY_SEAL_NACK_WRONG_KIND       = 5u;
inline constexpr std::uint32_t LEGACY_SEAL_NACK_NOT_FOREVER      = 6u;
inline constexpr std::uint32_t LEGACY_SEAL_NACK_ALREADY_SEALED   = 7u;
inline constexpr std::uint32_t LEGACY_SEAL_NACK_DISCARD_FAIL     = 9u;

enum class ShopItemSealOutcome : std::uint8_t {
    Success             = 0,
    NotUsableSeal       = 1,
    NotUsableTarget     = 2,
    NotFound            = 3,
    WrongSealItem       = 4,
    WrongKind           = 5,
    NotForever          = 6,
    AlreadySealed       = 7,
    DiscardFail         = 9,
    NoPlayer            = 100,
};

struct ShopItemSealValidationInput final {
    bool player_found = false;
    bool seal_item_usable = false;
    bool target_item_usable = false;
    bool seal_item_resolved = false;
    bool target_item_resolved = false;
    bool target_item_info_resolved = false;
    bool seal_is_item_seal_kind = false;
    bool target_kind_ok = false;
    bool target_sell_price_forever = false;
    bool target_already_sealed = false;
    int  discard_rt = 0;
};

inline ShopItemSealOutcome classify_shop_item_seal_outcome(
    const ShopItemSealValidationInput& in) noexcept {
    if (!in.player_found) {
        return ShopItemSealOutcome::NoPlayer;
    }
    if (!in.seal_item_usable) {
        return ShopItemSealOutcome::NotUsableSeal;
    }
    if (!in.target_item_usable) {
        return ShopItemSealOutcome::NotUsableTarget;
    }
    if (!in.seal_item_resolved || !in.target_item_resolved ||
        !in.target_item_info_resolved) {
        return ShopItemSealOutcome::NotFound;
    }
    if (!in.seal_is_item_seal_kind) {
        return ShopItemSealOutcome::WrongSealItem;
    }
    if (!in.target_kind_ok) {
        return ShopItemSealOutcome::WrongKind;
    }
    if (!in.target_sell_price_forever) {
        return ShopItemSealOutcome::NotForever;
    }
    if (in.target_already_sealed) {
        return ShopItemSealOutcome::AlreadySealed;
    }
    if (in.discard_rt != 0) {
        return ShopItemSealOutcome::DiscardFail;
    }
    return ShopItemSealOutcome::Success;
}

enum class ShopItemSealSideEffectKind : std::uint8_t {
    LogShopItemUse          = 0,
    SetItemParamSeal        = 1,
    DeleteUsingShopItemInfo = 2,
    ShopItemParamUpdateToDb = 3,
    ShopItemDeleteToDb      = 4,
    LogShopItemSeal         = 5,
    BroadcastUseAck         = 6,
    BroadcastSealAck        = 7,
    BroadcastSealNack       = 8,
};

struct ShopItemSealSideEffect final {
    ShopItemSealSideEffectKind kind =
        ShopItemSealSideEffectKind::BroadcastSealNack;
    std::uint32_t target_db_idx = 0;
    std::uint32_t target_item_param = 0;
    std::uint16_t seal_item_idx = 0;
    std::uint16_t seal_item_pos = 0;
    std::uint32_t nack_code = 0;
};

struct ShopItemSealSideEffectPlan final {
    std::vector<ShopItemSealSideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
    std::uint32_t nack_code = 0;
};

inline ShopItemSealSideEffectPlan shop_item_seal_side_effect_plan(
    const ShopItemSealValidationInput& in,
    std::uint32_t target_db_idx,
    std::uint16_t seal_item_idx,
    std::uint16_t seal_item_pos) {
    ShopItemSealSideEffectPlan plan;
    const ShopItemSealOutcome outcome =
        classify_shop_item_seal_outcome(in);
    if (outcome == ShopItemSealOutcome::NoPlayer) {
        return plan;
    }
    if (outcome == ShopItemSealOutcome::Success) {
        plan.send_ack = true;
        plan.effects.reserve(8u);

        ShopItemSealSideEffect log1{};
        log1.kind = ShopItemSealSideEffectKind::LogShopItemUse;
        plan.effects.push_back(log1);

        ShopItemSealSideEffect seal{};
        seal.kind = ShopItemSealSideEffectKind::SetItemParamSeal;
        seal.target_db_idx = target_db_idx;
        seal.target_item_param = LEGACY_ITEM_PARAM_SEAL;
        plan.effects.push_back(seal);

        ShopItemSealSideEffect del_use{};
        del_use.kind = ShopItemSealSideEffectKind::DeleteUsingShopItemInfo;
        plan.effects.push_back(del_use);

        ShopItemSealSideEffect upd_db{};
        upd_db.kind = ShopItemSealSideEffectKind::ShopItemParamUpdateToDb;
        upd_db.target_db_idx = target_db_idx;
        upd_db.target_item_param = LEGACY_ITEM_PARAM_SEAL;
        plan.effects.push_back(upd_db);

        ShopItemSealSideEffect del_db{};
        del_db.kind = ShopItemSealSideEffectKind::ShopItemDeleteToDb;
        del_db.target_db_idx = target_db_idx;
        plan.effects.push_back(del_db);

        ShopItemSealSideEffect log2{};
        log2.kind = ShopItemSealSideEffectKind::LogShopItemSeal;
        plan.effects.push_back(log2);

        ShopItemSealSideEffect useack{};
        useack.kind = ShopItemSealSideEffectKind::BroadcastUseAck;
        useack.seal_item_idx = seal_item_idx;
        useack.seal_item_pos = seal_item_pos;
        plan.effects.push_back(useack);

        ShopItemSealSideEffect sealack{};
        sealack.kind = ShopItemSealSideEffectKind::BroadcastSealAck;
        plan.effects.push_back(sealack);
        return plan;
    }
    plan.send_nack = true;
    plan.effects.reserve(1u);
    ShopItemSealSideEffect nack{};
    nack.kind = ShopItemSealSideEffectKind::BroadcastSealNack;
    nack.seal_item_idx = seal_item_idx;
    nack.seal_item_pos = seal_item_pos;
    switch (outcome) {
    case ShopItemSealOutcome::NotUsableSeal:
        nack.nack_code = LEGACY_SEAL_NACK_NOT_USABLE_SEAL; break;
    case ShopItemSealOutcome::NotUsableTarget:
        nack.nack_code = LEGACY_SEAL_NACK_NOT_USABLE_TARGET; break;
    case ShopItemSealOutcome::NotFound:
        nack.nack_code = LEGACY_SEAL_NACK_NOT_FOUND; break;
    case ShopItemSealOutcome::WrongSealItem:
        nack.nack_code = LEGACY_SEAL_NACK_WRONG_SEAL_ITEM; break;
    case ShopItemSealOutcome::WrongKind:
        nack.nack_code = LEGACY_SEAL_NACK_WRONG_KIND; break;
    case ShopItemSealOutcome::NotForever:
        nack.nack_code = LEGACY_SEAL_NACK_NOT_FOREVER; break;
    case ShopItemSealOutcome::AlreadySealed:
        nack.nack_code = LEGACY_SEAL_NACK_ALREADY_SEALED; break;
    case ShopItemSealOutcome::DiscardFail:
        nack.nack_code = LEGACY_SEAL_NACK_DISCARD_FAIL; break;
    default:
        break;
    }
    plan.nack_code = nack.nack_code;
    plan.effects.push_back(nack);
    return plan;
}

}  // namespace mxh::server
