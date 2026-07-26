// item_manager_test.cpp

#include "mxh/server/item_manager.hpp"
#include "mxh/server/player_state.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::ItemOpResult;
using mxh::server::equip_item;
using mxh::server::unequip_item;
using mxh::server::add_item;
using mxh::server::remove_item;
using mxh::server::find_free_inventory_slot;
using mxh::server::pyoguk_in;
using mxh::server::pyoguk_out;
using mxh::server::add_money;
using mxh::server::spend_money;
using mxh::server::inventory_free_count;
using mxh::game::ItemBase;
using mxh::server::PlayerState;
using mxh::server::CalcBaseStats;

static ItemBase make_item(std::uint32_t db_idx, std::uint16_t icon) {
    ItemBase i;
    i.dwDBIdx = db_idx;
    i.wIconIdx = icon;
    return i;
}

static PlayerState make_player_for_item_test() {
    CalcBaseStats b; b.level = 10;
    return mxh::server::make_player_state(1, 1, 10, b, mxh::server::CalcEquipBonuses{});
}
}

// ---- add_item / find_free_inventory_slot ----
TEST(AddItem, FindsFreeSlot) {
    PlayerState s = make_player_for_item_test();
    EXPECT_TRUE(add_item(s.inventory, make_item(1, 100)).success);
    EXPECT_EQ(find_free_inventory_slot(s.inventory).value(), 1u);
}

TEST(AddItem, FullInventoryFails) {
    PlayerState s = make_player_for_item_test();
    // Fill all 80 slots
    for (std::uint16_t i = 0; i < s.inventory.items.size(); ++i) {
        EXPECT_TRUE(add_item(s.inventory, make_item(i + 1, 100)).success);
    }
    auto r = add_item(s.inventory, make_item(999, 100));
    EXPECT_FALSE(r.success);
    EXPECT_EQ(r.overflow, 1u);
}

TEST(AddItem, FreeCountMatchesEmpty) {
    PlayerState s = make_player_for_item_test();
    EXPECT_EQ(inventory_free_count(s.inventory), 80u);
    add_item(s.inventory, make_item(1, 100));
    EXPECT_EQ(inventory_free_count(s.inventory), 79u);
}

// ---- remove_item ----
TEST(RemoveItem, RemovesOccupiedSlot) {
    PlayerState s = make_player_for_item_test();
    add_item(s.inventory, make_item(1, 100));
    EXPECT_TRUE(remove_item(s.inventory, 0).success);
    EXPECT_EQ(inventory_free_count(s.inventory), 80u);
}

TEST(RemoveItem, EmptySlotFails) {
    PlayerState s = make_player_for_item_test();
    EXPECT_FALSE(remove_item(s.inventory, 5).success);
}

TEST(RemoveItem, OutOfRangeFails) {
    PlayerState s = make_player_for_item_test();
    EXPECT_FALSE(remove_item(s.inventory, 99).success);
}

// ---- equip_item ----
TEST(EquipItem, EmptySlotReceivesItem) {
    PlayerState s = make_player_for_item_test();
    ItemBase weapon = make_item(101, 42);
    EXPECT_TRUE(equip_item(s.equipment, s, weapon, 0).success);
    EXPECT_EQ(s.equipment.items[0].dwDBIdx, 101u);
}

TEST(EquipItem, DisplacesOldItemToInventory) {
    PlayerState s = make_player_for_item_test();
    ItemBase old_weapon = make_item(101, 42);
    ItemBase new_weapon = make_item(202, 50);
    equip_item(s.equipment, s, old_weapon, 0);
    EXPECT_TRUE(equip_item(s.equipment, s, new_weapon, 0).success);
    EXPECT_EQ(s.equipment.items[0].dwDBIdx, 202u);
    EXPECT_EQ(inventory_free_count(s.inventory), 79u);  // displaced to slot 0
    EXPECT_EQ(s.inventory.items[0].dwDBIdx, 101u);
}

TEST(EquipItem, InventoryFullRollback) {
    PlayerState s = make_player_for_item_test();
    ItemBase old_weapon = make_item(101, 42);
    ItemBase new_weapon = make_item(202, 50);
    equip_item(s.equipment, s, old_weapon, 0);
    // Fill inventory
    for (std::uint16_t i = 0; i < s.inventory.items.size(); ++i) {
        add_item(s.inventory, make_item(1000 + i, 100));
    }
    auto r = equip_item(s.equipment, s, new_weapon, 0);
    EXPECT_FALSE(r.success);
    // Slot kept old weapon
    EXPECT_EQ(s.equipment.items[0].dwDBIdx, 101u);
}

TEST(EquipItem, OutOfRangeSlotFails) {
    PlayerState s = make_player_for_item_test();
    EXPECT_FALSE(equip_item(s.equipment, s, make_item(1, 1), 99).success);
}

// ---- unequip_item ----
TEST(UnequipItem, MovesToInventory) {
    PlayerState s = make_player_for_item_test();
    equip_item(s.equipment, s, make_item(101, 42), 0);
    EXPECT_TRUE(unequip_item(s.equipment, s.inventory, 0).success);
    EXPECT_EQ(s.equipment.items[0].dwDBIdx, 0u);
    EXPECT_EQ(s.inventory.items[0].dwDBIdx, 101u);
}

TEST(UnequipItem, EmptySlotFails) {
    PlayerState s = make_player_for_item_test();
    EXPECT_FALSE(unequip_item(s.equipment, s.inventory, 0).success);
}

TEST(UnequipItem, FullInventoryFails) {
    PlayerState s = make_player_for_item_test();
    equip_item(s.equipment, s, make_item(101, 42), 0);
    for (std::uint16_t i = 0; i < s.inventory.items.size(); ++i) {
        add_item(s.inventory, make_item(1000 + i, 100));
    }
    EXPECT_FALSE(unequip_item(s.equipment, s.inventory, 0).success);
    // Item stays equipped
    EXPECT_EQ(s.equipment.items[0].dwDBIdx, 101u);
}

// ---- pyoguk_in / pyoguk_out ----
TEST(PyogukIn, EmptySlotReceivesItem) {
    PlayerState s = make_player_for_item_test();
    EXPECT_TRUE(pyoguk_in(s.pyoguk, make_item(101, 42)).success);
    EXPECT_EQ(s.pyoguk.items[0].dwDBIdx, 101u);
}

TEST(PyogukIn, FullFails) {
    PlayerState s = make_player_for_item_test();
    for (std::uint16_t i = 0; i < s.pyoguk.items.size(); ++i) {
        EXPECT_TRUE(pyoguk_in(s.pyoguk, make_item(100 + i, 1)).success);
    }
    EXPECT_FALSE(pyoguk_in(s.pyoguk, make_item(999, 1)).success);
}

TEST(PyogukOut, RemovesItem) {
    PlayerState s = make_player_for_item_test();
    pyoguk_in(s.pyoguk, make_item(101, 42));
    EXPECT_TRUE(pyoguk_out(s.pyoguk, 0).success);
    EXPECT_EQ(s.pyoguk.items[0].dwDBIdx, 0u);
}

// ---- add_money / spend_money ----
TEST(AddMoney, NormalAddition) {
    auto r = add_money(100, 50);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 150u);
}

TEST(AddMoney, OverflowReturnsNullopt) {
    EXPECT_FALSE(add_money(2000000000u - 10, 100).has_value());
}

TEST(SpendMoney, NormalSubtraction) {
    auto r = spend_money(100, 50);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 50u);
}

TEST(SpendMoney, InsufficientReturnsNullopt) {
    EXPECT_FALSE(spend_money(10, 50).has_value());
}

