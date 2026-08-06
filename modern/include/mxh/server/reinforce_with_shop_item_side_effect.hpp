//
// CItemManager::MP_ITEM_REINFORCE_WITHSHOPITEM_SYN from legacy
// [Server]Map/ItemManager.cpp:4814-4850.
//
// The legacy handler is a thin wrapper around ReinforceItemWithShopItem:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer (return if null).
//   2. rt = ReinforceItemWithShopItem(pPlayer, wTargetItemIdx,
//      TargetPos, wShopItemIdx, ShopItemPos, JewelWhich, wJewelUnit).
//   3. If rt == EI_TRUE: NO message sent (silent success).
//   4. Else if rt == 99: rewrite pmsg->Protocol to
//      MP_ITEM_REINFORCE_FAILED_ACK and SendAckMsg to player.
//   5. Else: MSG_ITEM_ERROR {REINFORCE_WITHSHOPITEM_NACK, dwObjectID,
//      rt} SendErrorMsg to player.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy EI_TRUE return code for ReinforceItemWithShopItem.
inline constexpr int LEGACY_REINFORCE_EI_TRUE = 0;

// 1:1 with legacy rt == 99 magic value (reinforce failed).
inline constexpr int LEGACY_REINFORCE_FAILED_RT = 99;

enum class ReinforceWithShopItemOutcome : std::uint8_t {
    Success   = 0,  // legacy: rt == EI_TRUE
    FailedAck = 1,  // legacy: rt == 99
    Nack      = 2,  // legacy: rt is any other error code
};

struct ReinforceWithShopItemValidationInput final {
    int rt = 0;  // return code from ReinforceItemWithShopItem
};

inline ReinforceWithShopItemOutcome classify_reinforce_with_shop_item_outcome(
    const ReinforceWithShopItemValidationInput& in) noexcept {
    if (in.rt == LEGACY_REINFORCE_EI_TRUE) {
        return ReinforceWithShopItemOutcome::Success;
    }
    if (in.rt == LEGACY_REINFORCE_FAILED_RT) {
        return ReinforceWithShopItemOutcome::FailedAck;
    }
    return ReinforceWithShopItemOutcome::Nack;
}

enum class ReinforceWithShopItemSideEffectKind : std::uint8_t {
    SendFailedAckToPlayer = 0,  // legacy MP_ITEM_REINFORCE_FAILED_ACK
    SendNackToPlayer      = 1,  // legacy MP_ITEM_REINFORCE_WITHSHOPITEM_NACK
};

struct ReinforceWithShopItemSideEffect final {
    ReinforceWithShopItemSideEffectKind kind =
        ReinforceWithShopItemSideEffectKind::SendFailedAckToPlayer;
    std::uint32_t player_id = 0;
    int nack_error_code = 0;
};

struct ReinforceWithShopItemSideEffectPlan final {
    std::vector<ReinforceWithShopItemSideEffect> effects;
    bool send_failed_ack = false;
    bool send_nack = false;
    int nack_error_code = 0;
};

inline ReinforceWithShopItemSideEffectPlan
reinforce_with_shop_item_side_effect_plan(
    const ReinforceWithShopItemValidationInput& in,
    std::uint32_t player_id) {
    ReinforceWithShopItemSideEffectPlan plan;
    const ReinforceWithShopItemOutcome outcome =
        classify_reinforce_with_shop_item_outcome(in);

    if (outcome == ReinforceWithShopItemOutcome::Success) {
        return plan;  // legacy: no message sent
    }
    if (outcome == ReinforceWithShopItemOutcome::FailedAck) {
        plan.send_failed_ack = true;
        plan.effects.reserve(1u);
        ReinforceWithShopItemSideEffect eff{};
        eff.kind =
            ReinforceWithShopItemSideEffectKind::SendFailedAckToPlayer;
        eff.player_id = player_id;
        plan.effects.push_back(eff);
        return plan;
    }
    plan.send_nack = true;
    plan.nack_error_code = in.rt;
    plan.effects.reserve(1u);
    ReinforceWithShopItemSideEffect eff{};
    eff.kind = ReinforceWithShopItemSideEffectKind::SendNackToPlayer;
    eff.player_id = player_id;
    eff.nack_error_code = in.rt;
    plan.effects.push_back(eff);
    return plan;
}

}  // namespace mxh::server
