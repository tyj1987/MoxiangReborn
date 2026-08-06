//
// CItemManager::MP_ITEMEXT_SHOPITEM_DECORATION_ON from legacy
// [Server]Map/ItemManager.cpp:6578-6594.
//
// The legacy handler is a simple broadcast:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer; if null, return.
//   2. Build MSG_DWORD2 {MP_ITEMEXT, DECORATION_ON, dwObjectID,
//      dwData1, dwData2}.
//   3. PACKEDDATA_OBJ->QuickSendExceptObjectSelf(pPlayer, ...).

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

enum class DecorationOnOutcome : std::uint8_t {
    Broadcast = 0,
    NoPlayer  = 1,
};

struct DecorationOnValidationInput final {
    bool player_found = false;
};

inline DecorationOnOutcome classify_decoration_on_outcome(
    const DecorationOnValidationInput& in) noexcept {
    if (!in.player_found) {
        return DecorationOnOutcome::NoPlayer;
    }
    return DecorationOnOutcome::Broadcast;
}

enum class DecorationOnSideEffectKind : std::uint8_t {
    BroadcastToOthers = 0,  // legacy QuickSendExceptObjectSelf
};

struct DecorationOnSideEffect final {
    DecorationOnSideEffectKind kind =
        DecorationOnSideEffectKind::BroadcastToOthers;
    std::uint32_t player_id = 0;
    std::uint32_t data1 = 0;
    std::uint32_t data2 = 0;
};

struct DecorationOnSideEffectPlan final {
    std::vector<DecorationOnSideEffect> effects;
    bool broadcast = false;
};

inline DecorationOnSideEffectPlan decoration_on_side_effect_plan(
    const DecorationOnValidationInput& in,
    std::uint32_t player_id,
    std::uint32_t data1,
    std::uint32_t data2) {
    DecorationOnSideEffectPlan plan;
    const DecorationOnOutcome outcome =
        classify_decoration_on_outcome(in);
    if (outcome == DecorationOnOutcome::NoPlayer) {
        return plan;
    }
    plan.broadcast = true;
    plan.effects.reserve(1u);
    DecorationOnSideEffect eff{};
    eff.kind = DecorationOnSideEffectKind::BroadcastToOthers;
    eff.player_id = player_id;
    eff.data1 = data1;
    eff.data2 = data2;
    plan.effects.push_back(eff);
    return plan;
}

}  // namespace mxh::server
