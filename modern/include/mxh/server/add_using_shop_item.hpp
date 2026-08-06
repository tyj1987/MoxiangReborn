// 1:1 data-plane port of CShopItemManager::AddUsingShopItem from legacy
// [Server]Map/ShopItemManager.cpp:2775-2781. Splits the legacy function
// (which is just an m_UsingItemPool->Alloc + memcpy + m_UsingItemTable.Add)
// into:
//   1. Pure data plane (this header): given a fully-built
//      SHOPITEMWITHTIME row + a key (dwItemIndex), produce the
//      UsingShopItemEntry that the table stores, returning whether the
//      row would be accepted (icon key not already present, key != 0).
//   2. Orchestrator half: the legacy function calls
//      m_UsingItemPool->Alloc() + memcpy + m_UsingItemTable.Add() in
//      one step; the modern port keeps the data-plane decision pure
//      and lets the orchestrator apply it to a ShopItemManager.
//
// 1:1 invariants:
//   - ItemIdx (the key) is the dwDBIdx of the row when present, but the
//     legacy code uses AddShopItem.ShopItem.ItemBase.wIconIdx as the
//     key when called from UseShopItem (the canonical use site). The
//     data plane therefore accepts the key as input and lets callers
//     pick the right value.
//   - Insertion is rejected when the key is already in the table
//     (legacy: m_UsingItemTable.Add returns false on dup key).

#pragma once

#include <cstdint>

#include <mxh/game/shop_item_types.hpp>

namespace mxh::server {

// 1:1 with legacy CShopItemManager::AddUsingShopItem(SHOPITEMWITHTIME*,
// WORD). Returns the constructed UsingShopItemEntry when the row would
// be inserted; returns std::nullopt when the key is zero or already
// present in the table (1:1 with legacy m_UsingItemTable.Add dup-key
// guard). The caller can read the pre-built SHOPITEMWITHTIME bytes
// from out->Data and the ItemIdx key from out->ItemIdx without any
// further lookups.
struct AddUsingShopItemEntry {
    std::uint64_t item_idx = 0;  // legacy dwItemIndex
    game::ShopItemWithTime data{};
};

enum class AddUsingShopItemStatus : std::uint8_t {
    Ok = 0,
    KeyZero,        // legacy uses zero-key guard
    AlreadyPresent, // legacy m_UsingItemTable.Add dup-key guard
};

struct AddUsingShopItemDecision final {
    AddUsingShopItemStatus status = AddUsingShopItemStatus::KeyZero;
    AddUsingShopItemEntry entry{};
};

// 1:1 with legacy CShopItemManager::AddUsingShopItem data plane.
// The decision contains the new entry bytes that the legacy code would
// have written into the pool + table. The orchestrator applies the
// insertion via ShopItemManager::add_using_item(entry).
//
// has_key returns true iff a row with the requested key is already in
// the table. The data plane does not own the table; the caller wires
// the ShopItemManager lookup into this predicate.
inline AddUsingShopItemDecision add_using_shop_item_decision(
    const game::ShopItemWithTime& row,
    std::uint16_t dw_item_index,
    bool already_present) {
    AddUsingShopItemDecision out;
    if (dw_item_index == 0u) {
        out.status = AddUsingShopItemStatus::KeyZero;
        return out;
    }
    if (already_present) {
        out.status = AddUsingShopItemStatus::AlreadyPresent;
        return out;
    }
    out.status = AddUsingShopItemStatus::Ok;
    out.entry.item_idx = static_cast<std::uint64_t>(dw_item_index);
    out.entry.data = row;
    return out;
}

}  // namespace mxh::server
