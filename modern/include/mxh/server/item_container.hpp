// item_container.hpp - 1:1 port of legacy item slots.
//
// Three slot types from [CC]Header/CommonStruct.h wire format:
//   - ITEM_TOTALINFO Inventory[80]   -- in-bag (cumulative)
//   - ITEM_TOTALINFO WearedItem[10] -- equipment positions
//   - ITEM_TOTALINFO ShopInventory[20]
//
// Plus the auxiliary PYOGUK (warehouse) + PET slots. The legacy
// engine keeps these arrays in monolithic Player struct; the modern
// port keeps them in dedicated containers so the wire-format is
// shared but the ownership is testable in isolation.

#pragma once

#include "mxh/game/item_types.hpp"
#include <array>
#include <cstdint>
#include <vector>

namespace mxh::server {

// One inventory item slot (=legacy CItem*).
inline mxh::game::ItemBase make_empty_slot() noexcept {
    return mxh::game::make_empty_item();
}

// Inventory (80 slots) — flat array of ItemBase.
struct InventoryContainer final {
    std::array<mxh::game::ItemBase, mxh::game::SLOT_INVENTORY_NUM> slots{};
    InventoryContainer() noexcept { for (auto& s : slots) s = make_empty_slot(); }
    std::size_t first_empty_slot() const noexcept;
    bool insert(const mxh::game::ItemBase& item) noexcept;
    bool remove(std::uint16_t position) noexcept;
    mxh::game::ItemBase* find_by_dbidx(std::uint32_t db_idx) noexcept;
};

// Wear (10 equipment slots, one per eWearedItem).
struct WearContainer final {
    std::array<mxh::game::ItemBase, mxh::game::WEARED_ITEM_MAX> slots{};
    WearContainer() noexcept { for (auto& s : slots) s = make_empty_slot(); }
    bool equip(std::uint8_t position, const mxh::game::ItemBase& item) noexcept;  // legacy WearItem
    bool unequip(std::uint8_t position) noexcept;
};

// Shop inventory (20 slots).
struct ShopInven final {
    std::array<mxh::game::ItemBase, mxh::game::TABCELL_SHOPINVEN_NUM> slots{};
    ShopInven() noexcept { for (auto& s : slots) s = make_empty_slot(); }
    bool add(const mxh::game::ItemBase& item) noexcept;
    bool remove(std::uint16_t position) noexcept;
};

// Pyoguk (warehouse): 150 slots per character. Legacy uses a separate
// grid per warehouse number; modern port keeps the same wire layout.
inline constexpr std::uint16_t PYOGUK_SLOT_NUM =
    static_cast<std::uint16_t>(mxh::game::SLOT_PYOGUK_NUM);
static_assert(PYOGUK_SLOT_NUM == 150);
struct PyogukContainer final {
    std::array<mxh::game::ItemBase, PYOGUK_SLOT_NUM> slots{};
    PyogukContainer() noexcept { for (auto& s : slots) s = make_empty_slot(); }
    bool deposit(const mxh::game::ItemBase& item) noexcept;
    bool withdraw(std::uint16_t position) noexcept;
};

}  // namespace mxh::server
