#pragma once

// ============================================================================
// Item data structures — 1:1 with original CommonStruct.h
//
// ITEMBASE layout (22 bytes, packed):
//   dwDBIdx(4) wIconIdx(2) Position(2) Durability(4) RareIdx(4)
//   QuickPosition(2) ItemParam(4)
//
// ITEM_TOTALINFO contains:
//   Inventory[80] + WearedItem[10] + ShopInventory[20]
// ============================================================================

#include <cstdint>
#include <cstring>
#include <vector>

namespace mxh::game {

// Slot counts (from CommonGameDefine.h)
constexpr int SLOT_INVENTORY_NUM    = 80;   // 4 tabs × 20 slots
constexpr int WEARED_ITEM_MAX      = 10;   // hat, weapon, dress, shoes, ring×2, cape, necklace, armlet, belt
constexpr int TABCELL_SHOPINVEN_NUM = 20;   // shop inventory slots
constexpr int SLOT_PYOGUK_NUM       = 20;   // warehouse slots

// Equipment positions (EWEARED_ITEM enum)
constexpr std::uint8_t WEARED_HAT      = 0;
constexpr std::uint8_t WEARED_WEAPON   = 1;
constexpr std::uint8_t WEARED_DRESS   = 2;
constexpr std::uint8_t WEARED_SHOES   = 3;
constexpr std::uint8_t WEARED_RING1   = 4;
constexpr std::uint8_t WEARED_RING2   = 5;
constexpr std::uint8_t WEARED_CAPE    = 6;
constexpr std::uint8_t WEARED_NECKLACE= 7;
constexpr std::uint8_t WEARED_ARMLET  = 8;
constexpr std::uint8_t WEARED_BELT    = 9;

// Position ranges (from CommonGameDefine.h TP_* constants)
constexpr std::uint16_t TP_INVENTORY_START    = 0;
constexpr std::uint16_t TP_INVENTORY_END      = TP_INVENTORY_START + SLOT_INVENTORY_NUM;       // 80
constexpr std::uint16_t TP_WEAREDITEM_START    = TP_INVENTORY_END;                               // 80
constexpr std::uint16_t TP_WEAREDITEM_END     = TP_WEAREDITEM_START + WEARED_ITEM_MAX;         // 90
constexpr std::uint16_t TP_SHOPINVEN_START    = TP_WEAREDITEM_END;                              // 90
constexpr std::uint16_t TP_SHOPINVEN_END      = TP_SHOPINVEN_START + TABCELL_SHOPINVEN_NUM;    // 110
constexpr std::uint16_t TP_PYOGUK_START       = TP_SHOPINVEN_END;                               // 110
constexpr std::uint16_t TP_PYOGUK_END         = TP_PYOGUK_START + SLOT_PYOGUK_NUM;              // 130

#pragma pack(push, 1)

// ICONBASE (8 bytes) — from CommonStruct.h
struct IconBase {
    std::uint32_t dwDBIdx;       // database unique ID
    std::uint16_t wIconIdx;      // item type icon index (0 = empty)
    std::uint16_t Position;       // slot position
};

// ITEMBASE (22 bytes) — from CommonStruct.h
struct ItemBase {
    std::uint32_t dwDBIdx;       // database unique ID (0 = no item)
    std::uint16_t wIconIdx;      // item type index (0 = empty slot)
    std::uint16_t Position;      // absolute position in inventory/equipment
    std::uint32_t Durability;    // current durability
    std::uint32_t RareIdx;       // rare option index (0 = no rare)
    std::uint16_t QuickPosition; // quick slot position (0xFFFF = none)
    std::uint32_t ItemParam;     // stack count / charge count
};

static_assert(sizeof(ItemBase) == 22, "ItemBase must be 22 bytes to match original");

// ITEM_TOTALINFO — sent as MP_ITEM_TOTALINFO_LOCAL payload
// Layout: Inventory[80] + WearedItem[10] + ShopInventory[20]
// Total size: 110 × 22 = 2420 bytes
struct ItemTotalInfo {
    ItemBase Inventory[SLOT_INVENTORY_NUM];      // 80 items
    ItemBase WearedItem[WEARED_ITEM_MAX];        // 10 equipment slots
    ItemBase ShopInventory[TABCELL_SHOPINVEN_NUM]; // 20 shop slots
};

static_assert(sizeof(ItemTotalInfo) == 22 * 110, "ItemTotalInfo size mismatch");

// SEND_ITEM_TOTALINFO_LOCAL — message payload for MP_ITEM_TOTALINFO_LOCAL
// Format: MONEYTYPE(4B) + ITEM_TOTALINFO(2420B) = 2424 bytes
struct SendItemTotalInfoLocal {
    std::uint32_t Money;
    ItemTotalInfo Items;
};

static_assert(sizeof(SendItemTotalInfoLocal) == 4 + 22 * 110, "SendItemTotalInfoLocal size mismatch");

#pragma pack(pop)

// Helper: create an empty item slot
inline ItemBase make_empty_item() {
    ItemBase item{};
    item.dwDBIdx = 0;
    item.wIconIdx = 0;
    item.Position = 0;
    item.Durability = 0;
    item.RareIdx = 0;
    item.QuickPosition = 0xFFFF;
    item.ItemParam = 0;
    return item;
}

// Helper: create a basic item
inline ItemBase make_item(std::uint32_t db_idx, std::uint16_t icon_idx,
                          std::uint16_t position, std::uint32_t dur = 100,
                          std::uint32_t count = 1) {
    ItemBase item{};
    item.dwDBIdx = db_idx;
    item.wIconIdx = icon_idx;
    item.Position = position;
    item.Durability = dur;
    item.RareIdx = 0;
    item.QuickPosition = 0xFFFF;
    item.ItemParam = count;
    return item;
}

// Helper: check if item slot is empty
inline bool is_empty_slot(const ItemBase& item) {
    return item.dwDBIdx == 0 || item.wIconIdx == 0;
}

}  // namespace mxh::game
