//
// Generic MP_ITEMEXT_*_RELEASE slot-unlock side-effect dispatcher.
//
// The legacy handlers MP_ITEMEXT_UNIQUEITEM_MIX_RELEASE and
// MP_ITEMEXT_SHOPITEM_CURSE_CANCELLATION_RELEASE
// ([Server]Map/ItemManager.cpp:6231-6243 and 6383-6395) share the same
// simple unlock pattern:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer; if null, return.
//   2. CItemSlot* pSlot = pPlayer->GetSlot(pmsg->wData).
//   3. If pSlot: pSlot->SetLock(pmsg->wData, FALSE).

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

enum class ReleaseSlotLockOutcome : std::uint8_t {
    Unlocked = 0,  // legacy: player found and slot exists
    NoPlayer = 1,  // legacy: FindUser returned null
    NoSlot   = 2,  // legacy: GetSlot returned null
};

struct ReleaseSlotLockValidationInput final {
    bool player_found = false;
    bool slot_exists = false;
};

inline ReleaseSlotLockOutcome classify_release_slot_lock_outcome(
    const ReleaseSlotLockValidationInput& in) noexcept {
    if (!in.player_found) {
        return ReleaseSlotLockOutcome::NoPlayer;
    }
    if (!in.slot_exists) {
        return ReleaseSlotLockOutcome::NoSlot;
    }
    return ReleaseSlotLockOutcome::Unlocked;
}

enum class ReleaseSlotLockSideEffectKind : std::uint8_t {
    SetSlotUnlock = 0,  // legacy pSlot->SetLock(wData, FALSE)
};

struct ReleaseSlotLockSideEffect final {
    ReleaseSlotLockSideEffectKind kind =
        ReleaseSlotLockSideEffectKind::SetSlotUnlock;
    std::uint32_t player_id = 0;
    std::uint16_t slot_pos = 0;
};

struct ReleaseSlotLockSideEffectPlan final {
    std::vector<ReleaseSlotLockSideEffect> effects;
    bool set_unlock = false;
};

inline ReleaseSlotLockSideEffectPlan release_slot_lock_side_effect_plan(
    const ReleaseSlotLockValidationInput& in,
    std::uint32_t player_id,
    std::uint16_t slot_pos) {
    ReleaseSlotLockSideEffectPlan plan;
    const ReleaseSlotLockOutcome outcome =
        classify_release_slot_lock_outcome(in);
    if (outcome != ReleaseSlotLockOutcome::Unlocked) {
        return plan;
    }
    plan.set_unlock = true;
    plan.effects.reserve(1u);
    ReleaseSlotLockSideEffect eff{};
    eff.kind = ReleaseSlotLockSideEffectKind::SetSlotUnlock;
    eff.player_id = player_id;
    eff.slot_pos = slot_pos;
    plan.effects.push_back(eff);
    return plan;
}

}  // namespace mxh::server
