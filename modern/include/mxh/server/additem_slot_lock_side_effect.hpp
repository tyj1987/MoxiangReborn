//
// Generic MP_ITEMEXT_*_ADDITEM_SYN slot-lock side-effect dispatcher.
//
// The legacy handlers MP_ITEMEXT_SHOPITEM_CURSE_CANCELLATION_ADDITEM_SYN
// and MP_ITEMEXT_UNIQUEITEM_MIX_ADDITEM_SYN
// ([Server]Map/ItemManager.cpp:6183-6228 and 6336-6381) share the same
// 3-gate validation pattern:
//   1. GetTableIdxPosition(ItemInfo.Position) == eItemTable_Inventory
//      -> error code 1.
//   2. CHKRT->ItemOf(pPlayer, ItemInfo.Position, ItemInfo.wIconIdx,
//      0, 0, CB_EXIST|CB_ICONIDX) -> error code 2.
//   3. pSlot->IsLock(ItemInfo.Position) == FALSE -> error code 3.
//
// On success (error code 0):
//   - pSlot->SetLock(Position, TRUE).
//   - Send MSG_ITEM {MP_ITEMEXT, ADDITEM_ACK, ItemInfo}.
//
// On failure:
//   - Send MSG_WORD2 {MP_ITEMEXT, ADDITEM_NACK, Position, error code}.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/CommonGameDefine.h eItemTable_Inventory.
inline constexpr std::uint32_t LEGACY_EITEMTABLE_INVENTORY = 1u;

enum class AddItemSlotLockOutcome : std::uint8_t {
    Ack     = 0,  // legacy: all 3 gates pass
    Nack    = 1,  // legacy: any gate fails
    NoPlayer = 2, // legacy: FindUser returned null
};

enum class AddItemSlotLockError : std::uint8_t {
    None             = 0,  // success
    WrongTable       = 1,  // legacy gate 1
    ItemOfFailed     = 2,  // legacy gate 2
    SlotAlreadyLocked = 3, // legacy gate 3
};

struct AddItemSlotLockValidationInput final {
    bool player_found = false;
    bool position_in_inventory = false;
    bool item_of_check_passed = false;
    bool slot_unlocked = false;
};

inline AddItemSlotLockOutcome classify_additem_slot_lock_outcome(
    const AddItemSlotLockValidationInput& in) noexcept {
    if (!in.player_found) {
        return AddItemSlotLockOutcome::NoPlayer;
    }
    if (!in.position_in_inventory ||
        !in.item_of_check_passed ||
        !in.slot_unlocked) {
        return AddItemSlotLockOutcome::Nack;
    }
    return AddItemSlotLockOutcome::Ack;
}

inline AddItemSlotLockError additem_slot_lock_error_code(
    const AddItemSlotLockValidationInput& in) noexcept {
    if (!in.position_in_inventory) {
        return AddItemSlotLockError::WrongTable;
    }
    if (!in.item_of_check_passed) {
        return AddItemSlotLockError::ItemOfFailed;
    }
    if (!in.slot_unlocked) {
        return AddItemSlotLockError::SlotAlreadyLocked;
    }
    return AddItemSlotLockError::None;
}

enum class AddItemSlotLockSideEffectKind : std::uint8_t {
    SendAckToPlayer  = 0,  // legacy MSG_ITEM {ADDITEM_ACK, ItemInfo}
    SendNackToPlayer = 1,  // legacy MSG_WORD2 {ADDITEM_NACK, pos, code}
    SetSlotLock      = 2,  // legacy pSlot->SetLock(Position, TRUE)
};

struct AddItemSlotLockSideEffect final {
    AddItemSlotLockSideEffectKind kind =
        AddItemSlotLockSideEffectKind::SendAckToPlayer;
    std::uint32_t player_id = 0;
    std::uint16_t position = 0;
    std::uint16_t w_icon_idx = 0;
    std::uint8_t  error_code = 0;  // 0 = ack, 1/2/3 = nack reason
};

struct AddItemSlotLockSideEffectPlan final {
    std::vector<AddItemSlotLockSideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
    bool set_lock = false;
    std::uint8_t error_code = 0;
};

inline AddItemSlotLockSideEffectPlan additem_slot_lock_side_effect_plan(
    const AddItemSlotLockValidationInput& in,
    std::uint32_t player_id,
    std::uint16_t position,
    std::uint16_t w_icon_idx) {
    AddItemSlotLockSideEffectPlan plan;
    const AddItemSlotLockOutcome outcome =
        classify_additem_slot_lock_outcome(in);
    if (outcome == AddItemSlotLockOutcome::NoPlayer) {
        return plan;
    }
    if (outcome == AddItemSlotLockOutcome::Ack) {
        plan.send_ack = true;
        plan.set_lock = true;
        plan.effects.reserve(2u);
        AddItemSlotLockSideEffect lock{};
        lock.kind = AddItemSlotLockSideEffectKind::SetSlotLock;
        lock.player_id = player_id;
        lock.position = position;
        plan.effects.push_back(lock);
        AddItemSlotLockSideEffect ack{};
        ack.kind = AddItemSlotLockSideEffectKind::SendAckToPlayer;
        ack.player_id = player_id;
        ack.position = position;
        ack.w_icon_idx = w_icon_idx;
        plan.effects.push_back(ack);
        return plan;
    }
    plan.send_nack = true;
    plan.error_code = static_cast<std::uint8_t>(
        additem_slot_lock_error_code(in));
    plan.effects.reserve(1u);
    AddItemSlotLockSideEffect nack{};
    nack.kind = AddItemSlotLockSideEffectKind::SendNackToPlayer;
    nack.player_id = player_id;
    nack.position = position;
    nack.error_code = plan.error_code;
    plan.effects.push_back(nack);
    return plan;
}

}  // namespace mxh::server
