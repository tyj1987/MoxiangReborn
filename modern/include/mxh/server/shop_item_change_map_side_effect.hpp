// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_SHOPITEM_CHANGEMAP_SYN from legacy
// [Server]Map/ItemManager.cpp:5087-5093.
//
// The legacy handler is a no-op acknowledgement. It only validates
// that the player exists; otherwise it returns immediately without
// any network response. The actual map-change logic is handled by
// the map server when the client sends a separate MP_MOVE/change-map
// request, so this handler exists purely as a server-side receipt
// for the client to know the server saw the SHOPITEM_CHANGEMAP_SYN.
//
// The data plane captures this as a 2-way decision: either the
// player is found (consumed) or not (early return). There is no
// ACK/NACK emitted to the client.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_SHOPITEM_CHANGEMAP_SYN
// (single protocol code, no ACK/NACK pair).
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_CHANGEMAP_SYN = 73u;

enum class ShopItemChangeMapOutcome : std::uint8_t {
    Consumed = 0,  // legacy: pPlayer found, handler returns silently
    NoPlayer = 1,  // legacy: FindUser returned null, early return
};

struct ShopItemChangeMapValidationInput final {
    bool player_found = false;
};

inline ShopItemChangeMapOutcome classify_shop_item_change_map_outcome(
    const ShopItemChangeMapValidationInput& in) noexcept {
    if (!in.player_found) {
        return ShopItemChangeMapOutcome::NoPlayer;
    }
    return ShopItemChangeMapOutcome::Consumed;
}

enum class ShopItemChangeMapSideEffectKind : std::uint8_t {
    SilentConsume = 0,  // legacy: handler returns with no network I/O
};

struct ShopItemChangeMapSideEffect final {
    ShopItemChangeMapSideEffectKind kind =
        ShopItemChangeMapSideEffectKind::SilentConsume;
    std::uint32_t object_id = 0;  // legacy pmsg->dwObjectID
};

struct ShopItemChangeMapSideEffectPlan final {
    std::vector<ShopItemChangeMapSideEffect> effects;
    bool consume = false;
};

inline ShopItemChangeMapSideEffectPlan shop_item_change_map_side_effect_plan(
    const ShopItemChangeMapValidationInput& in,
    std::uint32_t object_id) {
    ShopItemChangeMapSideEffectPlan plan;
    const ShopItemChangeMapOutcome outcome =
        classify_shop_item_change_map_outcome(in);
    if (outcome != ShopItemChangeMapOutcome::Consumed) {
        return plan;
    }
    plan.consume = true;
    plan.effects.reserve(1u);
    ShopItemChangeMapSideEffect eff{};
    eff.kind = ShopItemChangeMapSideEffectKind::SilentConsume;
    eff.object_id = object_id;
    plan.effects.push_back(eff);
    return plan;
}

}  // namespace mxh::server
