// 1:1 data-plane + side-effect-dispatcher port of CItemManager's
// MP_ITEM_MIX_ADDITEM_SYN handler from legacy
// [Server]Map/ItemManager.cpp:4267-4346.
//
// The legacy handler runs 6 gates in order and routes to one of
// 7 branches (6 NACK error codes + 1 ACK):
//   1. GetTableIdxPosition(Pos) != eItemTable_Inventory -> NACK
//      with wErrorCode=1.
//   2. CHKRT->ItemOf check fails -> NACK with wErrorCode=2.
//   3. Slot is locked -> NACK with wErrorCode=3.
//   4. IsOptionItem returns true -> NACK with wErrorCode=4.
//   5. GetMixItemInfo returns null -> NACK with wErrorCode=5.
//   6. (GetItemKind != eYOUNGYAK_ITEM && != eEXTRA_ITEM_JEWEL
//      && Durability > 1) -> NACK with wErrorCode=6.
//   7. All gates pass -> set slot lock + send
//      MP_ITEM_MIX_ADDITEM_ACK with the input ItemInfo.
//
// The data plane below encodes the 7-way decision; the side-effect
// dispatcher emits the structured steps so the orchestrator can route
// each branch to its subsystem (SetSlotLock + BroadcastAck /
// BroadcastNack).

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_MIX_ADDITEM_ACK /
// _NACK. The Category is MP_ITEM.
inline constexpr std::uint8_t LEGACY_MP_ITEM_MIX_ADDITEM_ACK  = 66u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_MIX_ADDITEM_NACK = 67u;

// 1:1 with legacy [CC]Header/CommonStruct.h eItemTable_Inventory.
inline constexpr int LEGACY_EITEMTABLE_INVENTORY = 1;

// 1:1 with legacy eYOUNGYAK_ITEM / eEXTRA_ITEM_JEWEL ItemKind
// values. The legacy ItemKind enum varies by build; the data plane
// uses the most common Korean build values.
inline constexpr std::uint8_t LEGACY_EKIND_YOUNGYAK  = 18u;
inline constexpr std::uint8_t LEGACY_EKIND_JEWEL     = 24u;

// 1:1 with legacy error codes 1..6 (legacy uses sequential numbers).
inline constexpr std::uint16_t LEGACY_MIX_ADDITEM_ERR_NOT_IN_INVEN   = 1u;
inline constexpr std::uint16_t LEGACY_MIX_ADDITEM_ERR_ITEM_MISMATCH  = 2u;
inline constexpr std::uint16_t LEGACY_MIX_ADDITEM_ERR_SLOT_LOCKED    = 3u;
inline constexpr std::uint16_t LEGACY_MIX_ADDITEM_ERR_OPTION_ITEM    = 4u;
inline constexpr std::uint16_t LEGACY_MIX_ADDITEM_ERR_NO_MIX_INFO    = 5u;
inline constexpr std::uint16_t LEGACY_MIX_ADDITEM_ERR_NOT_MIXABLE    = 6u;

enum class ItemMixAddItemOutcome : std::uint8_t {
    Success          = 0,
    NotInInventory   = 1,
    ItemMismatch     = 2,
    SlotLocked       = 3,
    OptionItem       = 4,
    NoMixInfo        = 5,
    NotMixable       = 6,
};

struct ItemMixAddItemValidationInput final {
    int table_idx_position = 0;     // legacy GetTableIdxPosition(pos)
    bool item_of_passed = false;    // legacy CHKRT->ItemOf result
    bool slot_is_locked = false;    // legacy pSlot->IsLock(pos)
    bool is_option_item = false;    // legacy ITEMMGR->IsOptionItem(...)
    bool has_mix_info = false;      // legacy ITEMMGR->GetMixItemInfo != null
    std::uint8_t item_kind = 0;     // legacy GetItemKind(...)
    std::uint16_t durability = 0;   // legacy pmsg->ItemInfo.Durability
};

// Pure decision function. The caller passes the resolved boolean
// results of the 6 gates (no I/O).
inline ItemMixAddItemOutcome classify_item_mix_add_item_outcome(
    const ItemMixAddItemValidationInput& in) noexcept {
    if (in.table_idx_position != LEGACY_EITEMTABLE_INVENTORY) {
        return ItemMixAddItemOutcome::NotInInventory;
    }
    if (!in.item_of_passed) {
        return ItemMixAddItemOutcome::ItemMismatch;
    }
    if (in.slot_is_locked) {
        return ItemMixAddItemOutcome::SlotLocked;
    }
    if (in.is_option_item) {
        return ItemMixAddItemOutcome::OptionItem;
    }
    if (!in.has_mix_info) {
        return ItemMixAddItemOutcome::NoMixInfo;
    }
    if (in.item_kind != LEGACY_EKIND_YOUNGYAK &&
        in.item_kind != LEGACY_EKIND_JEWEL &&
        in.durability > 1u) {
        return ItemMixAddItemOutcome::NotMixable;
    }
    return ItemMixAddItemOutcome::Success;
}

enum class ItemMixAddItemSideEffectKind : std::uint8_t {
    SetSlotLock = 0,                // legacy pSlot->SetLock(pos, TRUE)
    BroadcastMixAddItemAck = 1,     // legacy SendMsg(MP_ITEM_MIX_ADDITEM_ACK)
    BroadcastMixAddItemNack = 2,    // legacy SendMsg(MP_ITEM_MIX_ADDITEM_NACK, wErrorCode)
};

struct ItemMixAddItemSideEffect final {
    ItemMixAddItemSideEffectKind kind =
        ItemMixAddItemSideEffectKind::SetSlotLock;
    std::uint16_t item_pos = 0;     // legacy pmsg->ItemInfo.Position
    std::uint16_t error_code = 0;   // legacy wErrorCode (NACK payload wData2)
};

struct ItemMixAddItemSideEffectPlan final {
    std::vector<ItemMixAddItemSideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
    std::uint16_t error_code = 0;
};

// 1:1 with legacy ItemManager::MP_ITEM_MIX_ADDITEM_SYN. The success
// plan emits two steps in legacy order: set slot lock first, then
// broadcast ACK. The failure plan emits a single NACK step with the
// error code captured.
inline ItemMixAddItemSideEffectPlan item_mix_add_item_side_effect_plan(
    const ItemMixAddItemValidationInput& in,
    std::uint16_t item_pos) {
    ItemMixAddItemSideEffectPlan plan;
    const ItemMixAddItemOutcome outcome =
        classify_item_mix_add_item_outcome(in);
    if (outcome == ItemMixAddItemOutcome::Success) {
        plan.send_ack = true;
        plan.effects.reserve(2u);
        ItemMixAddItemSideEffect lock{};
        lock.kind = ItemMixAddItemSideEffectKind::SetSlotLock;
        lock.item_pos = item_pos;
        plan.effects.push_back(lock);
        ItemMixAddItemSideEffect ack{};
        ack.kind = ItemMixAddItemSideEffectKind::BroadcastMixAddItemAck;
        ack.item_pos = item_pos;
        plan.effects.push_back(ack);
        return plan;
    }
    plan.send_nack = true;
    plan.effects.reserve(1u);
    ItemMixAddItemSideEffect nack{};
    nack.kind = ItemMixAddItemSideEffectKind::BroadcastMixAddItemNack;
    nack.item_pos = item_pos;
    nack.error_code = static_cast<std::uint16_t>(outcome);
    plan.error_code = nack.error_code;
    plan.effects.push_back(nack);
    return plan;
}

}  // namespace mxh::server
