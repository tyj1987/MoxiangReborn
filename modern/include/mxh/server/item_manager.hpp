// item_manager.hpp - 1:1 port of legacy [Server]Map/ItemManager.h
// (CItemManager + CItemContainer + CItemSlot + CInventorySlot). Modern
// port models the inventory / pyoguk / wear slot operations as POD
// structs + free functions on top of the wire-format ItemBase slot.

#pragma once

#include "mxh/game/item_types.hpp"
#include "mxh/server/player_state.hpp"
#include <cstdint>
#include <optional>

namespace mxh::server {

struct ItemOpResult final {
    bool success = false;
    std::uint32_t overflow = 0;
};

// ---- Equip management ----
ItemOpResult equip_item(EquipSlots& slots, PlayerState& state,
                        const mxh::game::ItemBase& item,
                        std::uint8_t slot_idx) noexcept;

ItemOpResult unequip_item(EquipSlots& slots, InventorySlots& inv,
                          std::uint8_t slot_idx) noexcept;

// ---- Inventory management ----
ItemOpResult add_item(InventorySlots& inv, const mxh::game::ItemBase& item) noexcept;
ItemOpResult remove_item(InventorySlots& inv, std::uint16_t pos) noexcept;
std::optional<std::uint16_t> find_free_inventory_slot(const InventorySlots& inv) noexcept;

// ---- Pyoguk (warehouse) ----
ItemOpResult pyoguk_in(PyogukSlots& p, const mxh::game::ItemBase& item) noexcept;
ItemOpResult pyoguk_out(PyogukSlots& p, std::uint16_t pos) noexcept;

// ---- Money handling ----
// 1:1 port of legacy CPurse::Add/Minus: only adds money if it doesnt
// overflow MAX_MONEY (legacy 2 billion cap).
std::optional<std::uint32_t> add_money(std::uint32_t purse, std::uint32_t delta) noexcept;
std::optional<std::uint32_t> spend_money(std::uint32_t purse, std::uint32_t cost) noexcept;

std::uint16_t inventory_free_count(const InventorySlots& inv) noexcept;

}  // namespace mxh::server
