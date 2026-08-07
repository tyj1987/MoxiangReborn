// additem_slot_lock_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// additem_slot_lock_side_effect_plan(). The data plane returns an
// empty plan (no player), a 2-step success chain (SetSlotLock ->
// ADDITEM_ACK), or a single NACK with the 3-way error code; this
// header walks the plan and dispatches each entry to a virtual
// AddItemSlotLockSideEffectSink.
//
// 1:1 invariants (1:1 with legacy MP_ITEMEXT_SHOPITEM_CURSE_
// CANCELLATION_ADDITEM_SYN / MP_ITEMEXT_UNIQUEITEM_MIX_ADDITEM_SYN
// from [Server]Map/ItemManager.cpp:6183-6228 and 6336-6381):
//   - 3 gates in order with error codes 1/2/3: position in
//     inventory / CHKRT ItemOf / slot unlocked.
//   - Success: SetLock(Position, TRUE) then MSG_ITEM {ADDITEM_ACK,
//     ItemInfo} in legacy order.
//
// Pattern mirrors item_mix_add_item_side_effect_runtime.hpp (D4.54)
// and the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/additem_slot_lock_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the AddItemSlotLock side-effect chain.
class AddItemSlotLockSideEffectSink {
public:
    virtual ~AddItemSlotLockSideEffectSink() = default;

    // Legacy: MSG_ITEM {MP_ITEMEXT, ADDITEM_ACK, ItemInfo}.
    virtual void send_ack_to_player(std::uint32_t player_id,
                                    std::uint16_t position,
                                    std::uint16_t w_icon_idx) = 0;

    // Legacy: MSG_WORD2 {MP_ITEMEXT, ADDITEM_NACK, Position,
    // error code}.
    virtual void send_nack_to_player(std::uint32_t player_id,
                                     std::uint16_t position,
                                     std::uint8_t error_code) = 0;

    // Legacy: pSlot->SetLock(Position, TRUE).
    virtual void set_slot_lock(std::uint32_t player_id,
                               std::uint16_t position) = 0;
};

struct AddItemSlotLockRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t acks_sent       = 0;
    std::size_t nacks_sent      = 0;
    std::size_t slot_locks      = 0;
    bool ack_flag_consumed  = false;
    bool nack_flag_consumed = false;
    bool lock_flag_consumed = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline AddItemSlotLockRuntimeOutcome apply_additem_slot_lock_side_effects(
    const AddItemSlotLockSideEffectPlan& plan,
    AddItemSlotLockSideEffectSink& sink) {
    AddItemSlotLockRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case AddItemSlotLockSideEffectKind::SendAckToPlayer:
            sink.send_ack_to_player(effect.player_id, effect.position,
                                    effect.w_icon_idx);
            ++out.acks_sent;
            ++out.effects_applied;
            break;
        case AddItemSlotLockSideEffectKind::SendNackToPlayer:
            sink.send_nack_to_player(effect.player_id, effect.position,
                                     effect.error_code);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        case AddItemSlotLockSideEffectKind::SetSlotLock:
            sink.set_slot_lock(effect.player_id, effect.position);
            ++out.slot_locks;
            ++out.effects_applied;
            break;
        }
    }
    out.ack_flag_consumed = plan.send_ack;
    out.nack_flag_consumed = plan.send_nack;
    out.lock_flag_consumed = plan.set_lock;
    return out;
}

}  // namespace mxh::server
