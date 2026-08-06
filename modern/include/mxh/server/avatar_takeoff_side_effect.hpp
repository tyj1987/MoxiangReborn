// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_SHOPITEM_AVATAR_TAKEOFF from legacy
// [Server]Map/ItemManager.cpp:5373-5393.
//
// The legacy handler takes off an avatar item. The flow is:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer (return if null).
//   2. IsUseAbleShopItem(pPlayer, wData1, wData2) - check if
//      the item is usable. If not, send NACK and return.
//   3. TakeOffAvatarItem(wData1, wData2) - if it returns false
//      (something blocked the take-off), send NACK.
//
// On success the legacy code is silent (no ACK sent). The client
// updates its UI based on inventory diff.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_SHOPITEM_AVATAR_USE_NACK.
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_AVATAR_USE_NACK = 84u;

enum class AvatarTakeoffOutcome : std::uint8_t {
    SilentSuccess = 0,  // legacy: usable + take-off ok
    NotUsable     = 1,  // legacy: IsUseAbleShopItem returned false
    TakeOffFailed = 2,  // legacy: TakeOffAvatarItem returned false
    NoPlayer      = 3,  // legacy: FindUser returned null
};

struct AvatarTakeoffValidationInput final {
    bool player_found = false;
    bool usable_shop_item = false;
    bool take_off_ok = false;
};

inline AvatarTakeoffOutcome classify_avatar_takeoff_outcome(
    const AvatarTakeoffValidationInput& in) noexcept {
    if (!in.player_found) {
        return AvatarTakeoffOutcome::NoPlayer;
    }
    if (!in.usable_shop_item) {
        return AvatarTakeoffOutcome::NotUsable;
    }
    if (!in.take_off_ok) {
        return AvatarTakeoffOutcome::TakeOffFailed;
    }
    return AvatarTakeoffOutcome::SilentSuccess;
}

enum class AvatarTakeoffSideEffectKind : std::uint8_t {
    BroadcastAvatarUseNack = 0,  // legacy SendMsg(MP_ITEM_SHOPITEM_AVATAR_USE_NACK)
    SilentSuccess          = 1,  // legacy: no network I/O
};

struct AvatarTakeoffSideEffect final {
    AvatarTakeoffSideEffectKind kind =
        AvatarTakeoffSideEffectKind::SilentSuccess;
    std::uint16_t item_idx = 0;  // legacy pmsg->wData1
    std::uint16_t item_pos = 0;  // legacy pmsg->wData2
};

struct AvatarTakeoffSideEffectPlan final {
    std::vector<AvatarTakeoffSideEffect> effects;
    bool send_nack = false;
    bool silent_success = false;
};

inline AvatarTakeoffSideEffectPlan avatar_takeoff_side_effect_plan(
    const AvatarTakeoffValidationInput& in,
    std::uint16_t item_idx,
    std::uint16_t item_pos) {
    AvatarTakeoffSideEffectPlan plan;
    const AvatarTakeoffOutcome outcome =
        classify_avatar_takeoff_outcome(in);
    if (outcome == AvatarTakeoffOutcome::NoPlayer) {
        return plan;
    }
    plan.effects.reserve(1u);
    AvatarTakeoffSideEffect eff{};
    eff.item_idx = item_idx;
    eff.item_pos = item_pos;
    if (outcome == AvatarTakeoffOutcome::SilentSuccess) {
        plan.silent_success = true;
        eff.kind = AvatarTakeoffSideEffectKind::SilentSuccess;
    } else {
        plan.send_nack = true;
        eff.kind = AvatarTakeoffSideEffectKind::BroadcastAvatarUseNack;
    }
    plan.effects.push_back(eff);
    return plan;
}

}  // namespace mxh::server
