// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_SHOPITEM_MPINFO from legacy
// [Server]Map/ItemManager.cpp:5095-5103.
//
// The legacy handler fires a DB query to load the player's
// move-point (saved-location) info. The flow is:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer (return if null).
//   2. SavedMovePointInfo(pPlayer->GetID()) - DB query.
//
// The handler does NOT send any ACK/NACK to the client; the data
// arrives later via MP_ITEM broadcasts from the DB callback.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_SHOPITEM_MPINFO
// (single protocol code, no ACK/NACK pair).
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_MPINFO = 79u;

enum class ShopItemMpInfoOutcome : std::uint8_t {
    Triggered = 0,  // legacy: player found, DB query fired
    NoPlayer  = 1,  // legacy: FindUser returned null
};

struct ShopItemMpInfoValidationInput final {
    bool player_found = false;
};

inline ShopItemMpInfoOutcome classify_shop_item_mpinfo_outcome(
    const ShopItemMpInfoValidationInput& in) noexcept {
    if (!in.player_found) {
        return ShopItemMpInfoOutcome::NoPlayer;
    }
    return ShopItemMpInfoOutcome::Triggered;
}

enum class ShopItemMpInfoSideEffectKind : std::uint8_t {
    FireSavedMovePointDbQuery = 0,  // legacy SavedMovePointInfo
};

struct ShopItemMpInfoSideEffect final {
    ShopItemMpInfoSideEffectKind kind =
        ShopItemMpInfoSideEffectKind::FireSavedMovePointDbQuery;
    std::uint32_t object_id = 0;  // legacy pPlayer->GetID()
};

struct ShopItemMpInfoSideEffectPlan final {
    std::vector<ShopItemMpInfoSideEffect> effects;
    bool trigger_db = false;
};

inline ShopItemMpInfoSideEffectPlan shop_item_mpinfo_side_effect_plan(
    const ShopItemMpInfoValidationInput& in,
    std::uint32_t object_id) {
    ShopItemMpInfoSideEffectPlan plan;
    const ShopItemMpInfoOutcome outcome =
        classify_shop_item_mpinfo_outcome(in);
    if (outcome != ShopItemMpInfoOutcome::Triggered) {
        return plan;
    }
    plan.trigger_db = true;
    plan.effects.reserve(1u);
    ShopItemMpInfoSideEffect db{};
    db.kind = ShopItemMpInfoSideEffectKind::FireSavedMovePointDbQuery;
    db.object_id = object_id;
    plan.effects.push_back(db);
    return plan;
}

}  // namespace mxh::server
