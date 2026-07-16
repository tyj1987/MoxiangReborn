// mxh/src/services/InventoryServiceImpl.hpp
// Phase 13.2: Real IInventoryService implementation backed by
// mxh::game::ItemTotalInfo (the wire-format struct used by the
// MapHandler for in-memory player state).
//
// Architecture (Phase 13 service model):
//   - The interface IInventoryService is the contract that all
//     dialogs depend on (header-only, see modern/include/mxh/services/).
//   - The mock implementation lives in modern/tests/unit/services/
//     and pins the contract via 5 gtest cases.
//   - THIS file is the first **real** implementation: it wraps a
//     reference to ItemTotalInfo and exposes the read paths the
//     Tier 2/3 dialogs need (CharacterDialog, InventoryExDialog,
//     QuickDialog, MugongDialog, ItemShopDialog, etc.).
//
// Why a separate class instead of a free function?
//   The service is per-player (the ItemTotalInfo reference is bound
//   to one specific player). The dialog holds the service for the
//   lifetime of the dialog, and the MapHandler swaps the service
//   when the player logs in/out. The interface is reference-typed
//   (not pointer-typed) so the dialog does not own the service —
//   the MapHandler does, and the dialog is just a consumer.
//
// Usage pattern (from a future CharacterDialog::UpdateInventory):
//   void CharacterDialog::UpdateInventory() {
//     if (!m_inventory) return;
//     for (std::uint8_t slot = 0; slot < WEARED_ITEM_MAX; ++slot) {
//       const auto* item = m_inventory->getWearedItem(slot);
//       if (item) m_ppStatic.weapon->SetStaticText(
//           std::to_string(item->wIconIdx));
//     }
//   }
//
// Threading: read-only access from the dialog; mutations happen
// only inside MapHandler's player_mu_ critical section. The
// dialog is expected to call these methods from the same thread
// that drives the dialog's update tick (the render thread for
// the modern client framework; or, for the server-side smoke
// test in this phase, the test thread). The mock test in
// services/real/ exercises the read paths only.

#pragma once

#include "mxh/services/IInventoryService.hpp"

#include "mxh/game/item_types.hpp"

namespace mxh::services {

class InventoryServiceImpl final : public IInventoryService {
public:
    // Bind the service to a specific player's ItemTotalInfo.
    // The reference must remain valid for the lifetime of the
    // service (the dialog holds the service, the MapHandler
    // owns the ItemTotalInfo via PlayerInfo).
    explicit InventoryServiceImpl(const mxh::game::ItemTotalInfo& items) noexcept
        : m_items(items) {}

    // ----- Inventory slots (0..79) -----

    const mxh::game::ItemBase* getItem(std::uint16_t pos) const noexcept override {
        if (pos >= mxh::game::SLOT_INVENTORY_NUM) return nullptr;
        const auto& slot = m_items.Inventory[pos];
        return is_empty(slot) ? nullptr : &m_items.Inventory[pos];
    }

    std::uint16_t occupiedSlotCount() const noexcept override {
        std::uint16_t n = 0;
        for (std::uint16_t i = 0; i < mxh::game::SLOT_INVENTORY_NUM; ++i) {
            if (!is_empty(m_items.Inventory[i])) ++n;
        }
        return n;
    }

    std::uint16_t totalCapacity() const noexcept override {
        return mxh::game::SLOT_INVENTORY_NUM;
    }

    // ----- Weared (equipped) items (slot 0..9) -----

    const mxh::game::ItemBase* getWearedItem(std::uint8_t slot) const noexcept override {
        if (slot >= mxh::game::WEARED_ITEM_MAX) return nullptr;
        const auto& item = m_items.WearedItem[slot];
        return is_empty(item) ? nullptr : &m_items.WearedItem[slot];
    }

    bool isWearedSlotOccupied(std::uint8_t slot) const noexcept override {
        if (slot >= mxh::game::WEARED_ITEM_MAX) return false;
        return !is_empty(m_items.WearedItem[slot]);
    }

    // ----- Queries -----

    std::optional<std::uint16_t> findItemByIconIdx(std::uint16_t wIconIdx) const noexcept override {
        for (std::uint16_t pos = 0; pos < mxh::game::SLOT_INVENTORY_NUM; ++pos) {
            if (!is_empty(m_items.Inventory[pos]) &&
                m_items.Inventory[pos].wIconIdx == wIconIdx) {
                return pos;
            }
        }
        return std::nullopt;
    }

    bool hasItem(std::uint16_t wIconIdx) const noexcept override {
        return findItemByIconIdx(wIconIdx).has_value();
    }

private:
    // 1:1 quirk (legacy): an "empty" item slot is identified by
    // dwDBIdx == 0. The wire-format ItemBase struct (CommonStruct.h
    // ITEMBASE) defaults all fields to 0 on construction, so any
    // slot that was never written to will have dwDBIdx == 0.
    // This is the same predicate the legacy engine uses
    // (see m_ItemContainer.cpp in 4DyuchiGRX_common).
    static bool is_empty(const mxh::game::ItemBase& item) noexcept {
        return item.dwDBIdx == 0;
    }

    const mxh::game::ItemTotalInfo& m_items;
};

}  // namespace mxh::services
