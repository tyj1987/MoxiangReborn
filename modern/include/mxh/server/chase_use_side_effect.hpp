// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_SHOPITEM_CHASEUSE_SYN from legacy
// [Server]Map/ItemManager.cpp:5485-5501.
//
// The legacy handler validates a chase (location-track) item usage.
// The flow is:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer (return if null).
//   2. GetShopItemManager()->GetUsingItemInfo(pmsg->wData1) - if
//      the player has the item equipped, send ACK; else NACK.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_SHOPITEM_CHASEUSE_ACK
// / _NACK.
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_CHASEUSE_ACK  = 86u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_CHASEUSE_NACK = 87u;

enum class ChaseUseOutcome : std::uint8_t {
    Ack      = 0,  // legacy: GetUsingItemInfo != null
    Nack     = 1,  // legacy: GetUsingItemInfo == null
    NoPlayer = 2,  // legacy: FindUser returned null
};

struct ChaseUseValidationInput final {
    bool player_found = false;
    bool has_using_item = false;
};

inline ChaseUseOutcome classify_chase_use_outcome(
    const ChaseUseValidationInput& in) noexcept {
    if (!in.player_found) {
        return ChaseUseOutcome::NoPlayer;
    }
    if (in.has_using_item) {
        return ChaseUseOutcome::Ack;
    }
    return ChaseUseOutcome::Nack;
}

enum class ChaseUseSideEffectKind : std::uint8_t {
    BroadcastChaseUseAck  = 0,  // legacy SendMsg(CHASEUSE_ACK)
    BroadcastChaseUseNack = 1,  // legacy SendMsg(CHASEUSE_NACK)
};

struct ChaseUseSideEffect final {
    ChaseUseSideEffectKind kind =
        ChaseUseSideEffectKind::BroadcastChaseUseAck;
    std::uint16_t item_idx = 0;  // legacy pmsg->wData1
    std::uint16_t item_pos = 0;  // legacy pmsg->wData2
};

struct ChaseUseSideEffectPlan final {
    std::vector<ChaseUseSideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
};

inline ChaseUseSideEffectPlan chase_use_side_effect_plan(
    const ChaseUseValidationInput& in,
    std::uint16_t item_idx,
    std::uint16_t item_pos) {
    ChaseUseSideEffectPlan plan;
    const ChaseUseOutcome outcome = classify_chase_use_outcome(in);
    if (outcome == ChaseUseOutcome::NoPlayer) {
        return plan;
    }
    plan.effects.reserve(1u);
    ChaseUseSideEffect eff{};
    eff.item_idx = item_idx;
    eff.item_pos = item_pos;
    if (outcome == ChaseUseOutcome::Ack) {
        plan.send_ack = true;
        eff.kind = ChaseUseSideEffectKind::BroadcastChaseUseAck;
    } else {
        plan.send_nack = true;
        eff.kind = ChaseUseSideEffectKind::BroadcastChaseUseNack;
    }
    plan.effects.push_back(eff);
    return plan;
}

}  // namespace mxh::server
