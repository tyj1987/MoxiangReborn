// titan_delegate_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// titan_delegate_side_effect_plan(). The data plane returns an empty
// plan (no player) or a single entry -- DelegateToTitanManager for
// the 12 TITANITEMMGR actions / ReleaseSlotLock for the 6 release
// actions; this header walks the plan and dispatches each entry to a
// virtual TitanDelegateSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager MP_ITEM_TITAN_*_SYN /
// _ADDITEM_SYN / _RELEASEITEM delegations from
// [Server]Map/ItemManager.cpp:6104-6167):
//   - FindUser null -> return (empty plan).
//   - *_SYN / *_ADDITEM_SYN -> TITANITEMMGR->Xxx(pPlayer, pmsg).
//   - *_RELEASEITEM -> slot SetLock(wData, FALSE).
//
// Pattern mirrors item_mix_release_side_effect_runtime.hpp (D4.57)
// and the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/titan_delegate_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the TitanDelegate side-effect chain.
class TitanDelegateSideEffectSink {
public:
    virtual ~TitanDelegateSideEffectSink() = default;

    // Legacy: TITANITEMMGR->TitanRegister / ... / TitanPartsMakeAdditem
    // (action selects the exact method).
    virtual void delegate_to_titan_manager(
        std::uint32_t player_id, TitanDelegateAction action) = 0;

    // Legacy: pSlot->SetLock(wData, FALSE) -- releases the lock bit on
    // the titan craft slot.
    virtual void release_slot_lock(std::uint32_t player_id,
                                   std::uint16_t slot_pos) = 0;
};

struct TitanDelegateRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t delegations      = 0;
    std::size_t releases         = 0;
    bool delegated_flag_consumed = false;
    bool release_flag_consumed   = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline TitanDelegateRuntimeOutcome apply_titan_delegate_side_effects(
    const TitanDelegateSideEffectPlan& plan,
    TitanDelegateSideEffectSink& sink) {
    TitanDelegateRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case TitanDelegateSideEffectKind::DelegateToTitanManager:
            sink.delegate_to_titan_manager(effect.player_id,
                                           effect.action);
            ++out.delegations;
            ++out.effects_applied;
            break;
        case TitanDelegateSideEffectKind::ReleaseSlotLock:
            sink.release_slot_lock(effect.player_id, effect.slot_pos);
            ++out.releases;
            ++out.effects_applied;
            break;
        }
    }
    out.delegated_flag_consumed = plan.delegated;
    out.release_flag_consumed = plan.release_lock;
    return out;
}

}  // namespace mxh::server
