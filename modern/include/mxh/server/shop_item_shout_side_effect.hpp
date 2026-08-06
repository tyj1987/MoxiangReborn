// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_SHOPITEM_SHOUT_SYN from legacy
// [Server]Map/ItemManager.cpp:6030-6070.
//
// The legacy handler broadcasts a world-shout message using a
// shout item. The flow is:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer (return if null).
//   2. !IsUseAbleShopItem -> NACK + return.
//   3. If the item is eSundries_Shout_Once / _Once_NoTrade:
//      a. DiscardItem -> if not EI_TRUE: NACK + return.
//      b. Send MP_ITEM_SHOPITEM_USE_ACK.
//   4. Send SEND_SHOUTBASE {MP_ITEM, SHOUT_ACK, characterIdx,
//      msg} to the agent server for world broadcast.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_SHOPITEM_SHOUT_ACK/_NACK.
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_SHOUT_ACK  = 91u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_SHOUT_NACK = 92u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_USE_ACK    = 93u;

// 1:1 with legacy eSundries_Shout_Once / _Once_NoTrade. The legacy
// uses two distinct IDs; both share the same data-plane branch.
inline constexpr std::uint16_t LEGACY_ESUNDRIES_SHOUT_ONCE         = 49;
inline constexpr std::uint16_t LEGACY_ESUNDRIES_SHOUT_ONCE_NOTRADE = 50;

enum class ShopItemShoutOutcome : std::uint8_t {
    Broadcast     = 0,  // legacy: usable + broadcast
    NotUsable     = 1,  // legacy: IsUseAbleShopItem returned false
    DiscardFail   = 2,  // legacy: DiscardItem returned non-EI_TRUE
    NoPlayer      = 3,  // legacy: FindUser returned null
};

struct ShopItemShoutValidationInput final {
    bool player_found = false;
    bool usable_shop_item = false;
    bool is_once_variant = false;
    int  discard_rt = 0;
};

inline ShopItemShoutOutcome classify_shop_item_shout_outcome(
    const ShopItemShoutValidationInput& in) noexcept {
    if (!in.player_found) {
        return ShopItemShoutOutcome::NoPlayer;
    }
    if (!in.usable_shop_item) {
        return ShopItemShoutOutcome::NotUsable;
    }
    if (in.is_once_variant && in.discard_rt != 0) {
        return ShopItemShoutOutcome::DiscardFail;
    }
    return ShopItemShoutOutcome::Broadcast;
}

enum class ShopItemShoutSideEffectKind : std::uint8_t {
    DiscardShoutItem = 0,    // legacy DiscardItem (once variant only)
    BroadcastUseAck   = 1,    // legacy SendAckMsg(SHOPITEM_USE_ACK)
    ForwardShoutAck   = 2,    // legacy Send2AgentServer(SHOUT_ACK)
    BroadcastShoutNack = 3,   // legacy SendMsg(SHOUT_NACK)
};

struct ShopItemShoutSideEffect final {
    ShopItemShoutSideEffectKind kind =
        ShopItemShoutSideEffectKind::BroadcastShoutNack;
    std::uint16_t item_idx = 0;
    std::uint16_t item_pos = 0;
    std::uint32_t character_idx = 0;
    bool is_once_variant = false;
};

struct ShopItemShoutSideEffectPlan final {
    std::vector<ShopItemShoutSideEffect> effects;
    bool send_nack = false;
    bool forward_shout = false;
};

inline ShopItemShoutSideEffectPlan shop_item_shout_side_effect_plan(
    const ShopItemShoutValidationInput& in,
    std::uint16_t item_idx,
    std::uint16_t item_pos,
    std::uint32_t character_idx) {
    ShopItemShoutSideEffectPlan plan;
    const ShopItemShoutOutcome outcome =
        classify_shop_item_shout_outcome(in);
    if (outcome == ShopItemShoutOutcome::NoPlayer) {
        return plan;
    }
    if (outcome == ShopItemShoutOutcome::NotUsable) {
        plan.send_nack = true;
        plan.effects.reserve(1u);
        ShopItemShoutSideEffect nack{};
        nack.kind = ShopItemShoutSideEffectKind::BroadcastShoutNack;
        nack.item_idx = item_idx;
        nack.item_pos = item_pos;
        plan.effects.push_back(nack);
        return plan;
    }
    if (outcome == ShopItemShoutOutcome::DiscardFail) {
        plan.send_nack = true;
        plan.effects.reserve(1u);
        ShopItemShoutSideEffect nack{};
        nack.kind = ShopItemShoutSideEffectKind::BroadcastShoutNack;
        nack.item_idx = item_idx;
        nack.item_pos = item_pos;
        nack.is_once_variant = true;
        plan.effects.push_back(nack);
        return plan;
    }
    // Broadcast
    plan.forward_shout = true;
    if (in.is_once_variant) {
        plan.effects.reserve(3u);
        ShopItemShoutSideEffect dis{};
        dis.kind = ShopItemShoutSideEffectKind::DiscardShoutItem;
        dis.item_idx = item_idx;
        dis.item_pos = item_pos;
        dis.is_once_variant = true;
        plan.effects.push_back(dis);

        ShopItemShoutSideEffect useack{};
        useack.kind = ShopItemShoutSideEffectKind::BroadcastUseAck;
        useack.item_idx = item_idx;
        useack.item_pos = item_pos;
        plan.effects.push_back(useack);
    } else {
        plan.effects.reserve(1u);
    }
    ShopItemShoutSideEffect bcast{};
    bcast.kind = ShopItemShoutSideEffectKind::ForwardShoutAck;
    bcast.item_idx = item_idx;
    bcast.item_pos = item_pos;
    bcast.character_idx = character_idx;
    plan.effects.push_back(bcast);
    return plan;
}

}  // namespace mxh::server
