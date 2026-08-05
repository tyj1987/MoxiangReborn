// Item lookup table for the modern server.  1:1 port of the legacy
// ITEMMGR->GetItemInfo(wIconIdx) path in
// [CC]Skill/ItemManager.h + [CC]Header/GameResourceStruct.h ITEM_INFO.
// Holds an ordered vector of ItemInfo plus an O(1)
// item_idx -> vector-index hash for fast lookups.  The 1:1 wire
// byte layout (fields, sizes) is locked by ITEM_INFO in
// modern/include/mxh/game/item_list_types.hpp.
//
// Phase D6.x adds init_from_bin(): loads the real legacy
// Resource/ItemList.bin (packed-text format) into the table.
//
// Convergence with item_effects.cpp: when an ItemManager is loaded,
// resolve_item_effect(w_icon_idx, manager) reads real LifeRecover /
// LifeRecoverRate / NaeRyukRecover / NaeRyukRecoverRate from the
// ITEM_INFO row matching the icon idx, instead of the hardcoded
// linear-scale placeholder.

#pragma once

#include "item_list_types.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace mxh::game {

// Out-of-range item access.
class ItemNotFound : public std::out_of_range {
public:
    explicit ItemNotFound(std::uint32_t item_idx)
        : std::out_of_range("ItemManager: item_idx " +
                            std::to_string(item_idx) +
                            " not found"),
          m_item_idx(item_idx) {}
    std::uint32_t item_idx() const noexcept { return m_item_idx; }
private:
    std::uint32_t m_item_idx;
};

class ItemManager {
public:
    ItemManager() = default;

    ItemManager(const ItemManager&) = delete;
    ItemManager& operator=(const ItemManager&) = delete;

    // Phase D6.x hook: load the real item table from a packed-text
    // ItemList.bin file (the 1:1 legacy file format).  Clears any
    // existing table on success.  Throws std::runtime_error on I/O
    // failure or header corruption; per-row parse errors are
    // accumulated and returned via out_errors (may be nullptr).
    // 1:1 with the legacy CItemManager::LoadItemInfo() in
    // [Server]Map/ItemManager.cpp::SetItemInfo lines 3956-4023.
    void init_from_bin(const std::string& path,
                       std::uint32_t* out_errors = nullptr);

    // Append a single ItemInfo to the table and update the index.
    // The item must not already exist (item_idx must be unique) --
    // callers are expected to enforce this for bin parsing.
    void add(const ItemInfo& it);
    void add(ItemInfo&& it);

    // Lookup.  Throws ItemNotFound if the item isnt in the table.
    const ItemInfo& get(std::uint32_t item_idx) const;

    // Non-throwing variant.  Returns true and populates out on
    // success; returns false and leaves out untouched on miss.
    bool try_get(std::uint32_t item_idx, ItemInfo& out) const noexcept;

    // Returns true iff item_idx is in the table.
    bool exists(std::uint32_t item_idx) const noexcept;

    // Number of items currently loaded.
    std::size_t size() const noexcept { return m_items.size(); }

    // Clear all items.  Mainly for tests.
    void clear() noexcept;

    // Read-only access to the underlying vector.
    const std::vector<ItemInfo>& items() const noexcept { return m_items; }

private:
    std::vector<ItemInfo> m_items;
    std::unordered_map<std::uint32_t, std::size_t> m_idx;
};

}  // namespace mxh::game
