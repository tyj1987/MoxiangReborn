#pragma once

// ============================================================================
// Item data structures — 1:1 with original CommonStruct.h
//
// ITEMBASE layout (22 bytes, packed):
//   dwDBIdx(4) wIconIdx(2) Position(2) Durability(4) RareIdx(4)
//   QuickPosition(2) ItemParam(4)
//
// ITEM_TOTALINFO contains:
//   Inventory[80] + WearedItem[10] + ShopInventory[20] + PetWear[3] + TitanWear[7] + TitanShop[4]
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace mxh::game {

// Slot counts (from CommonGameDefine.h)
constexpr int SLOT_INVENTORY_NUM = 80;
constexpr int WEARED_ITEM_MAX = 10;
constexpr int TAB_PYOGUK_NUM = 5;
constexpr int TABCELL_PYOGUK_NUM = 30;
constexpr int SLOT_PYOGUK_NUM = TAB_PYOGUK_NUM * TABCELL_PYOGUK_NUM;
constexpr int TAB_SHOPITEM_NUM = 5;
constexpr int TABCELL_SHOPITEM_NUM = 30;
constexpr int SLOT_SHOPITEM_NUM = TAB_SHOPITEM_NUM * TABCELL_SHOPITEM_NUM;
constexpr int TAB_SHOPINVEN_NUM = 2;
constexpr int TABCELL_SHOPINVEN_NUM = 20;
constexpr int SLOT_SHOPINVEN_NUM = TAB_SHOPINVEN_NUM * TABCELL_SHOPINVEN_NUM;
constexpr int SLOT_PETINVEN_NUM = 60;
constexpr int SLOT_PETWEAR_NUM = 3;
constexpr int SLOT_TITANWEAR_NUM = 7;
constexpr int SLOT_TITANSHOPITEM_NUM = 4;

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

// Position ranges from the default KOR/CHINA CommonGameDefine.h branch.
constexpr std::uint16_t TP_INVENTORY_START = 0;
constexpr std::uint16_t TP_INVENTORY_END = TP_INVENTORY_START + SLOT_INVENTORY_NUM;
constexpr std::uint16_t TP_WEAREDITEM_START = TP_INVENTORY_END;
constexpr std::uint16_t TP_WEAREDITEM_END = TP_WEAREDITEM_START + WEARED_ITEM_MAX;
constexpr std::uint16_t TP_PYOGUK_START = TP_WEAREDITEM_END;
constexpr std::uint16_t TP_PYOGUK_END = TP_PYOGUK_START + SLOT_PYOGUK_NUM;
constexpr std::uint16_t TP_SHOPITEM_START = TP_PYOGUK_END;
constexpr std::uint16_t TP_SHOPITEM_END = TP_SHOPITEM_START + SLOT_SHOPITEM_NUM;
constexpr std::uint16_t TP_SHOPINVEN_START = TP_SHOPITEM_END;
constexpr std::uint16_t TP_SHOPINVEN_END = TP_SHOPINVEN_START + SLOT_SHOPINVEN_NUM;
constexpr std::uint16_t TP_PETINVEN_START = TP_SHOPINVEN_END;
constexpr std::uint16_t TP_PETINVEN_END = TP_PETINVEN_START + SLOT_PETINVEN_NUM;
constexpr std::uint16_t TP_PETWEAR_START = TP_PETINVEN_END;
constexpr std::uint16_t TP_PETWEAR_END = TP_PETWEAR_START + SLOT_PETWEAR_NUM;
constexpr std::uint16_t TP_TITANWEAR_START = TP_PETWEAR_END;
constexpr std::uint16_t TP_TITANWEAR_END = TP_TITANWEAR_START + SLOT_TITANWEAR_NUM;
constexpr std::uint16_t TP_TITANSHOPITEM_START = TP_TITANWEAR_END;
constexpr std::uint16_t TP_TITANSHOPITEM_END = TP_TITANSHOPITEM_START + SLOT_TITANSHOPITEM_NUM;

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

// ITEM_TOTALINFO embedded in SEND_HERO_TOTALINFO.
// Default KOR/CHINA size: 124 × 22 = 2728 bytes.
struct ItemTotalInfo {
    ItemBase Inventory[SLOT_INVENTORY_NUM];
    ItemBase WearedItem[WEARED_ITEM_MAX];
    ItemBase ShopInventory[TABCELL_SHOPINVEN_NUM];
    ItemBase PetWearedItem[SLOT_PETWEAR_NUM];
    ItemBase TitanWearedItem[SLOT_TITANWEAR_NUM];
    ItemBase TitanShopItem[SLOT_TITANSHOPITEM_NUM];
};

inline constexpr std::size_t ITEM_TOTAL_SLOT_COUNT =
    SLOT_INVENTORY_NUM + WEARED_ITEM_MAX + TABCELL_SHOPINVEN_NUM +
    SLOT_PETWEAR_NUM + SLOT_TITANWEAR_NUM + SLOT_TITANSHOPITEM_NUM;

static_assert(ITEM_TOTAL_SLOT_COUNT == 124);
static_assert(sizeof(ItemTotalInfo) == 2728, "ItemTotalInfo must match legacy wire size");
static_assert(sizeof(ItemTotalInfo) == sizeof(ItemBase) * ITEM_TOTAL_SLOT_COUNT);
static_assert(offsetof(ItemTotalInfo, Inventory) == 0);
static_assert(offsetof(ItemTotalInfo, WearedItem) == 1760);
static_assert(offsetof(ItemTotalInfo, ShopInventory) == 1980);
static_assert(offsetof(ItemTotalInfo, PetWearedItem) == 2420);
static_assert(offsetof(ItemTotalInfo, TitanWearedItem) == 2486);
static_assert(offsetof(ItemTotalInfo, TitanShopItem) == 2640);

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
