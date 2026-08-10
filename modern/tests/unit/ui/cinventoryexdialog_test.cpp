#include "cinventoryexdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
class InventorySnapshot final : public mxh::services::IInventoryService {
public:
 mxh::game::ItemBase items[80]{};
 const mxh::game::ItemBase* getItem(std::uint16_t p) const noexcept override{return p<80&&items[p].dwDBIdx?&items[p]:nullptr;}
 std::uint16_t occupiedSlotCount() const noexcept override{return 1;}
 std::uint16_t totalCapacity() const noexcept override{return 80;}
 const mxh::game::ItemBase* getWearedItem(std::uint8_t) const noexcept override{return nullptr;}
 bool isWearedSlotOccupied(std::uint8_t) const noexcept override{return false;}
 std::optional<std::uint16_t> findItemByIconIdx(std::uint16_t icon) const noexcept{return icon==items[0].wIconIdx?std::optional<std::uint16_t>(0):std::nullopt;}
 bool hasItem(std::uint16_t icon) const noexcept override{return icon==items[0].wIconIdx;}
};
TEST(InventoryExDialog, AddsQueriesAndCountsSlots){cInventoryExDialog d;EXPECT_EQ(d.GetBlankNum(),60u);EXPECT_TRUE(d.AddItem(100,20));EXPECT_TRUE(d.IsExist(0));EXPECT_EQ(d.GetItemForPos(0)->item_id,100);EXPECT_EQ(d.GetBlankNum(),59u);}
TEST(InventoryExDialog, MovesAndDeletesItems){cInventoryExDialog d;d.AddItem(100);d.AddItem(200);EXPECT_TRUE(d.MoveItem(0,5));EXPECT_FALSE(d.IsExist(0));EXPECT_EQ(d.GetItemForPos(5)->item_id,100);EXPECT_TRUE(d.DeleteItem(5));EXPECT_FALSE(d.IsExist(5));}
TEST(InventoryExDialog, LockedItemCannotMove){cInventoryExDialog d;d.AddItem(100);ASSERT_TRUE(d.SetItemLocked(0,true));EXPECT_FALSE(d.MoveItem(0,1));}
TEST(InventoryExDialog, DurabilityMoneyStateAndRelease){cInventoryExDialog d;d.AddItem(100,10);EXPECT_TRUE(d.UpdateItemDurabilityAdd(0,-3));EXPECT_EQ(d.GetItemForPos(0)->durability,7);d.SetMoney(99);d.SetState(InventoryState::Deal);d.ReleaseInventory();EXPECT_EQ(d.GetMoney(),0u);EXPECT_EQ(d.GetBlankNum(),60u);EXPECT_EQ(d.GetState(),InventoryState::Default);}
TEST(InventoryExDialog, RefreshesFromLiveInventorySnapshot){cInventoryExDialog d;InventorySnapshot snapshot;snapshot.items[0]=mxh::game::make_item(42,321,0,77);d.SetInventoryService(&snapshot);d.RefreshFromInventoryService();ASSERT_TRUE(d.GetItemForPos(0).has_value());EXPECT_EQ(d.GetItemForPos(0)->item_id,321);EXPECT_EQ(d.GetItemForPos(0)->durability,77);}

