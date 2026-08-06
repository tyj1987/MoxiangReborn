// 1:1 side-effect-dispatcher port of CItemManager::MP_ITEM_MIX_RELEASEITEM
// from legacy [Server]Map/ItemManager.cpp:4348-4358.
//
// The legacy handler is a pure side-effect (no ACK/NACK):
//   1. FindUser(pmsg->dwObjectID) -> pPlayer (return if null).
//   2. pSlot = pPlayer->GetSlot(pmsg->wData).
//   3. If pSlot != null, pSlot->SetLock(pmsg->wData, FALSE).
//
// The position is the slot index inside the slot returned by
// GetSlot. The single side effect is to clear the lock bit on
// that slot position. The legacy handler does not write any
// network response.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_MIX_RELEASEITEM
// (single protocol code, no ACK/NACK pair).
inline constexpr std::uint8_t LEGACY_MP_ITEM_MIX_RELEASEITEM = 70u;

enum class ItemMixReleaseOutcome : std::uint8_t {
    Released = 0,
    NoSlot   = 1,
    NoPlayer = 2,
};

struct ItemMixReleaseValidationInput final {
    bool player_found = false;
    bool slot_resolved = false;
};

inline ItemMixReleaseOutcome classify_item_mix_release_outcome(
    const ItemMixReleaseValidationInput& in) noexcept {
    if (!in.player_found) {
        return ItemMixReleaseOutcome::NoPlayer;
    }
    if (!in.slot_resolved) {
        return ItemMixReleaseOutcome::NoSlot;
    }
    return ItemMixReleaseOutcome::Released;
}

enum class ItemMixReleaseSideEffectKind : std::uint8_t {
    ClearSlotLock = 0,
};

struct ItemMixReleaseSideEffect final {
    ItemMixReleaseSideEffectKind kind =
        ItemMixReleaseSideEffectKind::ClearSlotLock;
    std::uint16_t slot_pos = 0;
};

struct ItemMixReleaseSideEffectPlan final {
    std::vector<ItemMixReleaseSideEffect> effects;
    bool release_lock = false;
};

inline ItemMixReleaseSideEffectPlan item_mix_release_side_effect_plan(
    const ItemMixReleaseValidationInput& in,
    std::uint16_t slot_pos) {
    ItemMixReleaseSideEffectPlan plan;
    const ItemMixReleaseOutcome outcome =
        classify_item_mix_release_outcome(in);
    if (outcome != ItemMixReleaseOutcome::Released) {
        return plan;
    }
    plan.release_lock = true;
    plan.effects.reserve(1u);
    ItemMixReleaseSideEffect eff{};
    eff.kind = ItemMixReleaseSideEffectKind::ClearSlotLock;
    eff.slot_pos = slot_pos;
    plan.effects.push_back(eff);
    return plan;
}

}  // namespace mxh::server
