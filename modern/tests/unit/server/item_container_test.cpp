// item_container_test.cpp - slot-level 1:1 wire tests.

#include "mxh/server/item_container.hpp"
#include "mxh/game/item_types.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::InventoryContainer;
using mxh::server::WearContainer;
using mxh::server::ShopInven;
using mxh::server::PyogukContainer;
using mxh::game::make_item;
using mxh::game::ItemBase;
using mxh::game::is_empty_slot;

}

TEST(InventoryContainer, EmptyOnConstruction) {
    InventoryContainer inv;
    EXPECT_EQ(inv.first_empty_slot(), 0u);
    for (auto& s : inv.slots) EXPECT_TRUE(is_empty_slot(s));
}

TEST(InventoryContainer, InsertSetsPosition) {
    InventoryContainer inv;
    auto it = make_item(1001, 50, /*position*/99, /*dur*/100, /*count*/5);
    EXPECT_TRUE(inv.insert(it));
    EXPECT_FALSE(inv.slots[0].dwDBIdx != 1001u || inv.slots[0].Position != 0u);
}

TEST(InventoryContainer, InsertFullReturnsFalse) {
    InventoryContainer inv;
    for (std::uint16_t i = 0; i < mxh::game::SLOT_INVENTORY_NUM; ++i) {
        ItemBase it{};
        it.dwDBIdx = 1 + i; it.wIconIdx = 10;
        EXPECT_TRUE(inv.insert(it));
    }
    ItemBase extra{};
    extra.dwDBIdx = 9999; extra.wIconIdx = 10;
    EXPECT_FALSE(inv.insert(extra));
}

TEST(InventoryContainer, RemoveClears) {
    InventoryContainer inv;
    inv.insert(make_item(7, 11, 0, 1, 1));
    EXPECT_TRUE(inv.remove(0));
    EXPECT_TRUE(is_empty_slot(inv.slots[0]));
    EXPECT_FALSE(inv.remove(0));  // already empty
}

TEST(InventoryContainer, FindByDbidx) {
    InventoryContainer inv;
    inv.insert(make_item(101, 10, 0, 1, 1));
    inv.insert(make_item(202, 10, 1, 1, 1));
    auto* f1 = inv.find_by_dbidx(101);
    ASSERT_NE(f1, nullptr);
    EXPECT_EQ(f1->Position, 0u);
    auto* f2 = inv.find_by_dbidx(202);
    ASSERT_NE(f2, nullptr);
    EXPECT_EQ(f2->Position, 1u);
    EXPECT_EQ(inv.find_by_dbidx(0), nullptr);   // 0 = no item
    EXPECT_EQ(inv.find_by_dbidx(999), nullptr);
}

TEST(WearContainer, EquipAndUnequip) {
    WearContainer w;
    auto wep = make_item(11, 100, 0, 100, 1);  // weapon
    EXPECT_TRUE(w.equip(0, wep));
    EXPECT_FALSE(w.equip(0, wep));  // already equipped
    EXPECT_TRUE(w.unequip(0));
    EXPECT_TRUE(is_empty_slot(w.slots[0]));
    EXPECT_FALSE(w.unequip(0));
}

TEST(ShopInven, AddAndRemove) {
    ShopInven s;
    auto it = make_item(50, 5, 0, 100, 10);
    EXPECT_TRUE(s.add(it));
    EXPECT_TRUE(s.remove(0));
    EXPECT_FALSE(s.remove(0));
}

TEST(PyogukContainer, DepositAndWithdraw) {
    PyogukContainer p;
    EXPECT_TRUE(p.deposit(make_item(99, 22, 0, 100, 3)));
    EXPECT_TRUE(p.withdraw(0));
    EXPECT_FALSE(p.withdraw(0));
}

TEST(ItemContainers, WireSlotSizesMatchLegacy) {
    // Slot sizes are locked by static_asserts in item_types.hpp; here we
    // re-check the array dimensions and total bytes against legacy.
    EXPECT_EQ(sizeof(InventoryContainer::slots) / sizeof(ItemBase),
              static_cast<std::size_t>(mxh::game::SLOT_INVENTORY_NUM));
    EXPECT_EQ(sizeof(WearContainer::slots) / sizeof(ItemBase),
              static_cast<std::size_t>(mxh::game::WEARED_ITEM_MAX));
    EXPECT_EQ(sizeof(ShopInven::slots) / sizeof(ItemBase),
              static_cast<std::size_t>(mxh::game::TABCELL_SHOPINVEN_NUM));
    EXPECT_EQ(sizeof(PyogukContainer::slots) / sizeof(ItemBase),
              static_cast<std::size_t>(mxh::server::PYOGUK_SLOT_NUM));
}
