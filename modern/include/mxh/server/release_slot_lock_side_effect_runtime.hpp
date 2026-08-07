// release_slot_lock_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// release_slot_lock_side_effect_plan(). The data plane returns an
// empty plan (no player / no slot) or a single SetSlotUnlock entry;
// this header walks the plan and dispatches the entry to a virtual
// ReleaseSlotLockSideEffectSink.
//
// 1:1 invariants (1:1 with legacy MP_ITEMEXT_UNIQUEITEM_MIX_RELEASE /
// MP_ITEMEXT_SHOPITEM_CURSE_CANCELLATION_RELEASE from
// [Server]Map/ItemManager.cpp:6231-6243 and 6383-6395):
//   - FindUser null -> return; GetSlot(wData) null -> return.
//   - Both resolve -> pSlot->SetLock(wData, FALSE).
//
// Pattern mirrors item_mix_release_side_effect_runtime.hpp (D4.57)
// and the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/release_slot_lock_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ReleaseSlotLock side-effect chain.
class ReleaseSlotLockSideEffectSink {
public:
    virtual ~ReleaseSlotLockSideEffectSink() = default;

    // Legacy: pSlot->SetLock(wData, FALSE) -- clears the lock bit on
    // the craft slot.
    virtual void set_slot_unlock(std::uint32_t player_id,
                                 std::uint16_t slot_pos) = 0;
};

struct ReleaseSlotLockRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t unlocks         = 0;
    bool unlock_flag_consumed = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline ReleaseSlotLockRuntimeOutcome apply_release_slot_lock_side_effects(
    const ReleaseSlotLockSideEffectPlan& plan,
    ReleaseSlotLockSideEffectSink& sink) {
    ReleaseSlotLockRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ReleaseSlotLockSideEffectKind::SetSlotUnlock:
            sink.set_slot_unlock(effect.player_id, effect.slot_pos);
            ++out.unlocks;
            ++out.effects_applied;
            break;
        }
    }
    out.unlock_flag_consumed = plan.set_unlock;
    return out;
}

}  // namespace mxh::server
