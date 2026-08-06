//
// CItemManager::MP_ITEM_TITAN_*_SYN/_ADDITEM_SYN/_RELEASEITEM
// delegations from legacy
// [Server]Map/ItemManager.cpp:6104-6167.
//
// The Titan handlers in legacy are thin wrappers around TITANITEMMGR:
//   MP_ITEM_TITAN_REGISTER_SYN              -> TITANITEMMGR->TitanRegister
//   MP_ITEM_TITAN_REGISTER_ADDITEM_SYN      -> TITANITEMMGR->TitanRegisterAdditem
//   MP_ITEM_TITAN_REGISTER_RELEASEITEM      -> pSlot->SetLock(wData, FALSE)
//   MP_ITEM_TITAN_DISSOLUTION_SYN           -> TITANITEMMGR->TitanCancellation
//   MP_ITEM_TITAN_DISSOLUTION_ADDITEM_SYN   -> TITANITEMMGR->TitanDissolutionAdditem
//   MP_ITEM_TITAN_DISSOLUTION_RELEASEITEM   -> pSlot->SetLock(wData, FALSE)
//   MP_ITEM_TITANMIX_SYN                    -> TITANITEMMGR->TitanMix
//   MP_ITEM_TITANMIX_ADDITEM_SYN            -> TITANITEMMGR->TitanMixAdditem
//   MP_ITEM_TITANMIX_RELEASEITEM            -> pSlot->SetLock(wData, FALSE)
//   MP_ITEM_TITANUPGRADE_SYN                -> TITANITEMMGR->TitanUpgrade
//   MP_ITEM_TITANUPGRADE_ADDITEM_SYN        -> TITANITEMMGR->TitanUpgradeAdditem
//   MP_ITEM_TITANUPGRADE_RELEASEITEM        -> pSlot->SetLock(wData, FALSE)
//   MP_ITEM_TITANBREAK_SYN                  -> TITANITEMMGR->TitanBreak
//   MP_ITEM_TITANBREAK_ADDITEM_SYN          -> TITANITEMMGR->TitanBreakAdditem
//   MP_ITEM_TITANBREAK_RELEASEITEM          -> pSlot->SetLock(wData, FALSE)
//   MP_ITEM_TPM_SYN                         -> TITANITEMMGR->TitanPartsMake
//   MP_ITEM_TPM_ADDITEM_SYN                 -> TITANITEMMGR->TitanPartsMakeAdditem
//   MP_ITEM_TPM_RELEASEITEM                 -> pSlot->SetLock(wData, FALSE)
//
// All share the same pattern:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer; if null return.
//   2. For *_SYN / *_ADDITEM_SYN: delegate to the matching
//      TITANITEMMGR method with the (pPlayer, pmsg) arguments.
//   3. For *_RELEASEITEM: get slot by wData, SetLock(wData, FALSE).
//
// This side-effect dispatcher captures the (found, action) decision
// surface so we can drive the runtime from a pure function.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

enum class TitanDelegateAction : std::uint8_t {
    TitanRegister,
    TitanRegisterAdditem,
    TitanCancellation,
    TitanDissolutionAdditem,
    TitanMix,
    TitanMixAdditem,
    TitanUpgrade,
    TitanUpgradeAdditem,
    TitanBreak,
    TitanBreakAdditem,
    TitanPartsMake,
    TitanPartsMakeAdditem,
    ReleaseItemLock,
};

enum class TitanDelegateOutcome : std::uint8_t {
    Delegated = 0,  // legacy: player found + dispatch to TITANITEMMGR/Slot
    NoPlayer  = 1,  // legacy: FindUser returned null
};

struct TitanDelegateValidationInput final {
    bool player_found = false;
};

inline TitanDelegateOutcome classify_titan_delegate_outcome(
    const TitanDelegateValidationInput& in) noexcept {
    if (!in.player_found) {
        return TitanDelegateOutcome::NoPlayer;
    }
    return TitanDelegateOutcome::Delegated;
}

enum class TitanDelegateSideEffectKind : std::uint8_t {
    DelegateToTitanManager = 0,  // legacy TITANITEMMGR->Xxx(...)
    ReleaseSlotLock         = 1,  // legacy pSlot->SetLock(wData, FALSE)
};

struct TitanDelegateSideEffect final {
    TitanDelegateSideEffectKind kind =
        TitanDelegateSideEffectKind::DelegateToTitanManager;
    TitanDelegateAction action = TitanDelegateAction::TitanRegister;
    std::uint32_t player_id = 0;
    std::uint16_t slot_pos = 0;
};

struct TitanDelegateSideEffectPlan final {
    std::vector<TitanDelegateSideEffect> effects;
    bool delegated = false;
    bool release_lock = false;
    TitanDelegateAction action = TitanDelegateAction::TitanRegister;
};

inline TitanDelegateSideEffectPlan titan_delegate_side_effect_plan(
    const TitanDelegateValidationInput& in,
    TitanDelegateAction action,
    std::uint32_t player_id,
    std::uint16_t slot_pos) {
    TitanDelegateSideEffectPlan plan;
    plan.action = action;
    const TitanDelegateOutcome outcome =
        classify_titan_delegate_outcome(in);
    if (outcome == TitanDelegateOutcome::NoPlayer) {
        return plan;
    }
    plan.delegated = true;
    plan.effects.reserve(1u);
    TitanDelegateSideEffect eff{};
    if (action == TitanDelegateAction::ReleaseItemLock) {
        eff.kind = TitanDelegateSideEffectKind::ReleaseSlotLock;
        plan.release_lock = true;
    } else {
        eff.kind = TitanDelegateSideEffectKind::DelegateToTitanManager;
    }
    eff.action = action;
    eff.player_id = player_id;
    eff.slot_pos = slot_pos;
    plan.effects.push_back(eff);
    return plan;
}

}  // namespace mxh::server
