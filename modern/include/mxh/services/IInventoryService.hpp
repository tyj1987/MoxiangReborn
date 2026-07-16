// mxh/services/IInventoryService.hpp
// Phase 13 service interface for inventory access.
//
// Tier 3 dialog (CharacterDialog, InventoryExDialog, QuickDialog,
// MugongDialog, ItemShopDialog, etc.) currently reads inventory
// state via legacy global singletons (OBJECTMGR / ITEMMGR /
// GameIn->GetInventoryDialog()). This service is the modern
// replacement: dialog code takes an `IInventoryService*` (passed
// at construction or via dependency injection) and queries
// inventory state through it.
//
// The interface is deliberately minimal 鈥?only the read paths
// that the Tier 2/3 dialogs need. Write paths (MoveItem,
// UseItem, DropItem) are not yet in scope; they will be added
// when the first dialog actually needs them, with full
// transactional semantics on top of db_thread / ItemManager.
//
// All methods are const so a service implementation can be
// safely queried from multiple dialogs concurrently (assuming
// the underlying state is snapshot-consistent 鈥?a real impl
// may need to take a snapshot first).
//
// Usage pattern (from a future CharacterDialog::Update):
//   void CharacterDialog::updateInventoryDisplay() {
//     const auto* inv = m_inventoryService;  // injected at ctor
//     if (!inv) return;
//     for (std::uint8_t slot = 0; slot < WEARED_ITEM_MAX; ++slot) {
//       const auto* item = inv->getWearedItem(slot);
//       // ... render item icon, durability, etc.
//     }
//   }
//
// The mock implementation lives in
// `modern/tests/unit/services/inventory_service_test.cpp` and
// is the canonical reference for the behavior the real
// ItemManager-backed implementation must replicate.

#pragma once

#include "mxh/game/item_types.hpp"

#include <cstdint>
#include <optional>

namespace mxh::services {

class IInventoryService {
public:
    virtual ~IInventoryService() = default;

    // ----- Inventory slots (0..79) -----

    // Return the item at inventory position `pos`, or nullptr if
    // the slot is empty or `pos` is out of [0, 80). The returned
    // pointer is owned by the service 鈥?do not delete. The
    // pointer remains valid for the lifetime of the service.
    virtual const mxh::game::ItemBase* getItem(std::uint16_t pos) const noexcept = 0;

    // Number of occupied inventory slots (for progress UI like
    // cGuagen showing "X / 80 used").
    virtual std::uint16_t occupiedSlotCount() const noexcept = 0;

    // Total capacity (always SLOT_INVENTORY_NUM in the current
    // design, but the interface allows for future expansion 鈥?    // e.g. inventory upgrades purchased from the cash shop).
    virtual std::uint16_t totalCapacity() const noexcept = 0;

    // ----- Weared (equipped) items (slot 0..9) -----

    // Return the item weared in slot `slot`, or nullptr if
    // empty. `slot` must be in [0, 10).
    virtual const mxh::game::ItemBase* getWearedItem(std::uint8_t slot) const noexcept = 0;

    // Convenience: check if a weared slot is occupied.
    virtual bool isWearedSlotOccupied(std::uint8_t slot) const noexcept = 0;

    // ----- Queries -----

    // Find the first inventory slot containing an item with
    // `wIconIdx`, or std::nullopt if no such item is held.
    // Useful for "how many of item X does the player have"
    // style UIs.
    virtual std::optional<std::uint16_t> findItemByIconIdx(std::uint16_t wIconIdx) const noexcept = 0;

    // True if the player has at least one item with the given
    // icon idx. Equivalent to `findItemByIconIdx(idx).has_value()`
    // but expresses intent better at the call site.
    virtual bool hasItem(std::uint16_t wIconIdx) const noexcept = 0;
};

}  // namespace mxh::services
