// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_SHOPITEM_INFO_SYN from legacy
// [Server]Map/ItemManager.cpp:5021-5032.
//
// The legacy handler resets the player's shop-item-init flag and
// fires a DB query to re-load the shop item info. The flow is:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer (return if null).
//   2. pPlayer->SetShopItemInit(FALSE) - mark the player as
//      needs to re-init shop item state.
//   3. CharacterShopItemInfo(pPlayer->GetID(), 0) - DB query.
//
// The handler does NOT send any ACK/NACK to the client; the data
// arrives later via MP_ITEM_SHOPITEM broadcasts from the DB
// callback.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_SHOPITEM_INFO_SYN
// (single protocol code, no ACK/NACK pair on the SYN path).
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_INFO_SYN = 78u;

enum class ShopItemInfoOutcome : std::uint8_t {
    Triggered = 0,  // legacy: player found, init + DB query fired
    NoPlayer  = 1,  // legacy: FindUser returned null
};

struct ShopItemInfoValidationInput final {
    bool player_found = false;
};

inline ShopItemInfoOutcome classify_shop_item_info_outcome(
    const ShopItemInfoValidationInput& in) noexcept {
    if (!in.player_found) {
        return ShopItemInfoOutcome::NoPlayer;
    }
    return ShopItemInfoOutcome::Triggered;
}

enum class ShopItemInfoSideEffectKind : std::uint8_t {
    SetShopItemInit     = 0,  // legacy SetShopItemInit(FALSE)
    FireShopItemDbQuery = 1,  // legacy CharacterShopItemInfo
};

struct ShopItemInfoSideEffect final {
    ShopItemInfoSideEffectKind kind =
        ShopItemInfoSideEffectKind::SetShopItemInit;
    std::uint32_t object_id = 0;     // legacy pPlayer->GetID()
    std::uint32_t start_db_idx = 0;  // legacy 2nd arg to CharacterShopItemInfo
};

struct ShopItemInfoSideEffectPlan final {
    std::vector<ShopItemInfoSideEffect> effects;
    bool reset_init = false;
    bool trigger_db = false;
};

inline ShopItemInfoSideEffectPlan shop_item_info_side_effect_plan(
    const ShopItemInfoValidationInput& in,
    std::uint32_t object_id) {
    ShopItemInfoSideEffectPlan plan;
    const ShopItemInfoOutcome outcome =
        classify_shop_item_info_outcome(in);
    if (outcome != ShopItemInfoOutcome::Triggered) {
        return plan;
    }
    plan.reset_init = true;
    plan.trigger_db = true;
    plan.effects.reserve(2u);

    ShopItemInfoSideEffect init{};
    init.kind = ShopItemInfoSideEffectKind::SetShopItemInit;
    init.object_id = object_id;
    plan.effects.push_back(init);

    ShopItemInfoSideEffect db{};
    db.kind = ShopItemInfoSideEffectKind::FireShopItemDbQuery;
    db.object_id = object_id;
    db.start_db_idx = 0;
    plan.effects.push_back(db);
    return plan;
}

}  // namespace mxh::server
