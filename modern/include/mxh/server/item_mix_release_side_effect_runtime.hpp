// item_mix_release_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// item_mix_release_side_effect_plan(). The data plane returns an
// empty plan (no player / no slot) or a single ClearSlotLock entry;
// this header walks the plan and dispatches the entry to a virtual
// ItemMixReleaseSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_MIX_RELEASEITEM from [Server]Map/ItemManager.cpp:4348-4358):
//   - The handler is a pure side-effect (no ACK/NACK): FindUser ->
//     GetSlot -> SetLock(slot, FALSE).
//   - FindUser returns null: handler returns immediately (empty
//     plan).
//   - GetSlot returns null: handler returns immediately (empty
//     plan).
//   - Both resolve: ClearSlotLock clears the lock bit on the slot
//     position (pmsg->wData).
//
// Pattern mirrors the silent-branch handling in
// item_move_side_effect_runtime.hpp (D4.43) and the rest of the
// runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/item_mix_release_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ItemMixRelease side-effect chain.
class ItemMixReleaseSideEffectSink {
public:
    virtual ~ItemMixReleaseSideEffectSink() = default;

    // Legacy: pSlot->SetLock(wData, FALSE) -- clears the lock bit on
    // the slot position. No network message is emitted.
    virtual void clear_slot_lock(std::uint16_t slot_pos) = 0;
};

struct ItemMixReleaseRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t locks_cleared   = 0;
    bool release_flag_consumed  = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline ItemMixReleaseRuntimeOutcome apply_item_mix_release_side_effects(
    const ItemMixReleaseSideEffectPlan& plan,
    ItemMixReleaseSideEffectSink& sink) {
    ItemMixReleaseRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ItemMixReleaseSideEffectKind::ClearSlotLock:
            sink.clear_slot_lock(effect.slot_pos);
            ++out.locks_cleared;
            ++out.effects_applied;
            break;
        }
    }
    out.release_flag_consumed = plan.release_lock;
    return out;
}

}  // namespace mxh::server
