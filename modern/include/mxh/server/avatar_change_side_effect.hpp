// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_SHOPITEM_AVATAR_CHANGE from legacy
// [Server]Map/ItemManager.cpp:5394-5409.
//
// The legacy handler broadcasts an avatar-change event. The flow is:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer (return if null).
//   2. CalcShopItemOption(pmsg->wData2, TRUE) - recompute shop
//      item option stats based on the new avatar.
//   3. Broadcast MSG_DWORD2 {MP_ITEM, AVATAR_PUTON, pPlayer->GetID(),
//      pmsg->wData2} to all other map clients (except self).
//
// The handler does NOT send an ACK/NACK to the originating client;
// the broadcast is the response.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_SHOPITEM_AVATAR_PUTON.
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_AVATAR_PUTON = 85u;

enum class AvatarChangeOutcome : std::uint8_t {
    Broadcast = 0,  // legacy: player + option calc + broadcast
    NoPlayer  = 1,  // legacy: FindUser returned null
};

struct AvatarChangeValidationInput final {
    bool player_found = false;
};

inline AvatarChangeOutcome classify_avatar_change_outcome(
    const AvatarChangeValidationInput& in) noexcept {
    if (!in.player_found) {
        return AvatarChangeOutcome::NoPlayer;
    }
    return AvatarChangeOutcome::Broadcast;
}

enum class AvatarChangeSideEffectKind : std::uint8_t {
    RecalcShopItemOption  = 0,  // legacy CalcShopItemOption(..., TRUE)
    BroadcastAvatarPuton  = 1,  // legacy QuickSendExceptObjectSelf
};

struct AvatarChangeSideEffect final {
    AvatarChangeSideEffectKind kind =
        AvatarChangeSideEffectKind::RecalcShopItemOption;
    std::uint32_t object_id = 0;    // legacy pPlayer->GetID()
    std::uint16_t item_pos = 0;     // legacy pmsg->wData2
};

struct AvatarChangeSideEffectPlan final {
    std::vector<AvatarChangeSideEffect> effects;
    bool recalc = false;
    bool broadcast = false;
};

inline AvatarChangeSideEffectPlan avatar_change_side_effect_plan(
    const AvatarChangeValidationInput& in,
    std::uint32_t object_id,
    std::uint16_t item_pos) {
    AvatarChangeSideEffectPlan plan;
    const AvatarChangeOutcome outcome =
        classify_avatar_change_outcome(in);
    if (outcome != AvatarChangeOutcome::Broadcast) {
        return plan;
    }
    plan.recalc = true;
    plan.broadcast = true;
    plan.effects.reserve(2u);

    AvatarChangeSideEffect calc{};
    calc.kind = AvatarChangeSideEffectKind::RecalcShopItemOption;
    calc.object_id = object_id;
    calc.item_pos = item_pos;
    plan.effects.push_back(calc);

    AvatarChangeSideEffect bcast{};
    bcast.kind = AvatarChangeSideEffectKind::BroadcastAvatarPuton;
    bcast.object_id = object_id;
    bcast.item_pos = item_pos;
    plan.effects.push_back(bcast);
    return plan;
}

}  // namespace mxh::server
