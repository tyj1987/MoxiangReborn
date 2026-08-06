// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_SHOPITEM_NCHANGE_SYN from legacy
// [Server]Map/ItemManager.cpp:5546-5576.
//
// The legacy handler renames a character via a name-change shop
// item. The flow is:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer (return if null).
//   2. Linear search the player's shop inventory for the item
//      whose dwDBIdx matches pmsg->DBIdx.
//   3. If no match: send MP_ITEM_SHOPITEM_NCHANGE_NACK with
//      dwData = 6 (the legacy not found code).
//   4. Otherwise fire CharacterChangeName (DB call).

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_SHOPITEM_NCHANGE_NACK.
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_NCHANGE_NACK = 97u;

// 1:1 with legacy dwData=6 used as the item not found error code
// (per legacy [Server]Map/ItemManager.cpp:5569).
inline constexpr std::uint32_t LEGACY_NCHANGE_ERR_NOT_FOUND = 6u;

enum class ShopItemNameChangeOutcome : std::uint8_t {
    Triggered = 0,  // legacy: player + item found + DB call fired
    NotFound  = 1,  // legacy: item DBIdx not in shop inventory
    NoPlayer  = 2,  // legacy: FindUser returned null
};

struct ShopItemNameChangeValidationInput final {
    bool player_found = false;
    bool item_found = false;
};

inline ShopItemNameChangeOutcome classify_shop_item_name_change_outcome(
    const ShopItemNameChangeValidationInput& in) noexcept {
    if (!in.player_found) {
        return ShopItemNameChangeOutcome::NoPlayer;
    }
    if (!in.item_found) {
        return ShopItemNameChangeOutcome::NotFound;
    }
    return ShopItemNameChangeOutcome::Triggered;
}

enum class ShopItemNameChangeSideEffectKind : std::uint8_t {
    FireCharacterChangeNameDb = 0,  // legacy CharacterChangeName
    BroadcastNchangeNack       = 1,  // legacy SendMsg(NCHANGE_NACK, 6)
};

struct ShopItemNameChangeSideEffect final {
    ShopItemNameChangeSideEffectKind kind =
        ShopItemNameChangeSideEffectKind::FireCharacterChangeNameDb;
    std::uint32_t object_id = 0;     // legacy pmsg->dwObjectID
    std::uint32_t item_db_idx = 0;   // legacy pmsg->DBIdx
    std::uint32_t nack_code = 0;     // legacy dwData on NACK path
};

struct ShopItemNameChangeSideEffectPlan final {
    std::vector<ShopItemNameChangeSideEffect> effects;
    bool trigger_db = false;
    bool send_nack = false;
    std::uint32_t nack_code = 0;
};

inline ShopItemNameChangeSideEffectPlan
shop_item_name_change_side_effect_plan(
    const ShopItemNameChangeValidationInput& in,
    std::uint32_t object_id,
    std::uint32_t item_db_idx) {
    ShopItemNameChangeSideEffectPlan plan;
    const ShopItemNameChangeOutcome outcome =
        classify_shop_item_name_change_outcome(in);
    if (outcome == ShopItemNameChangeOutcome::NoPlayer) {
        return plan;
    }
    plan.effects.reserve(1u);
    ShopItemNameChangeSideEffect eff{};
    eff.object_id = object_id;
    eff.item_db_idx = item_db_idx;
    if (outcome == ShopItemNameChangeOutcome::Triggered) {
        plan.trigger_db = true;
        eff.kind = ShopItemNameChangeSideEffectKind::FireCharacterChangeNameDb;
    } else {
        plan.send_nack = true;
        plan.nack_code = LEGACY_NCHANGE_ERR_NOT_FOUND;
        eff.kind = ShopItemNameChangeSideEffectKind::BroadcastNchangeNack;
        eff.nack_code = LEGACY_NCHANGE_ERR_NOT_FOUND;
    }
    plan.effects.push_back(eff);
    return plan;
}

}  // namespace mxh::server
