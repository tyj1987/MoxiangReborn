// item_mix_add_item_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// item_mix_add_item_side_effect_plan(). The data plane returns a
// 2-step success chain (SetSlotLock -> BroadcastMixAddItemAck) or a
// single NACK entry with the 6-way error code; this header walks the
// plan and dispatches each entry to a virtual
// ItemMixAddItemSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_MIX_ADDITEM_SYN from
// [Server]Map/ItemManager.cpp:4267-4346):
//   - Gate 1: GetTableIdxPosition != eItemTable_Inventory -> NACK 1.
//   - Gate 2: CHKRT->ItemOf fails -> NACK 2.
//   - Gate 3: slot locked -> NACK 3.
//   - Gate 4: IsOptionItem true -> NACK 4.
//   - Gate 5: GetMixItemInfo null -> NACK 5.
//   - Gate 6: kind not {YOUNGYAK, JEWEL} && Durability > 1 -> NACK 6.
//   - Success: SetSlotLock(pos, TRUE) then MP_ITEM_MIX_ADDITEM_ACK
//     (66) in legacy order.
//
// Pattern mirrors item_mix_release_side_effect_runtime.hpp (D4.57)
// and the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/item_mix_add_item_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ItemMixAddItem side-effect chain.
class ItemMixAddItemSideEffectSink {
public:
    virtual ~ItemMixAddItemSideEffectSink() = default;

    // Legacy: pSlot->SetLock(pos, TRUE) -- locks the inventory slot
    // so the mix recipe cannot be disturbed mid-craft.
    virtual void set_slot_lock(std::uint16_t item_pos) = 0;

    // Legacy: SendMsg(MP_ITEM_MIX_ADDITEM_ACK) -- echoes the input
    // ItemInfo to the player.
    virtual void broadcast_mix_add_item_ack(std::uint16_t item_pos) = 0;

    // Legacy: SendMsg(MP_ITEM_MIX_ADDITEM_NACK, wErrorCode) -- the
    // error code discriminates which of the 6 gates failed.
    virtual void broadcast_mix_add_item_nack(
        std::uint16_t item_pos, std::uint16_t error_code) = 0;
};

struct ItemMixAddItemRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t slot_locks      = 0;
    std::size_t acks_sent       = 0;
    std::size_t nacks_sent      = 0;
    bool ack_flag_consumed  = false;
    bool nack_flag_consumed = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline ItemMixAddItemRuntimeOutcome apply_item_mix_add_item_side_effects(
    const ItemMixAddItemSideEffectPlan& plan,
    ItemMixAddItemSideEffectSink& sink) {
    ItemMixAddItemRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ItemMixAddItemSideEffectKind::SetSlotLock:
            sink.set_slot_lock(effect.item_pos);
            ++out.slot_locks;
            ++out.effects_applied;
            break;
        case ItemMixAddItemSideEffectKind::BroadcastMixAddItemAck:
            sink.broadcast_mix_add_item_ack(effect.item_pos);
            ++out.acks_sent;
            ++out.effects_applied;
            break;
        case ItemMixAddItemSideEffectKind::BroadcastMixAddItemNack:
            sink.broadcast_mix_add_item_nack(
                effect.item_pos, effect.error_code);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        }
    }
    out.ack_flag_consumed = plan.send_ack;
    out.nack_flag_consumed = plan.send_nack;
    return out;
}

}  // namespace mxh::server
