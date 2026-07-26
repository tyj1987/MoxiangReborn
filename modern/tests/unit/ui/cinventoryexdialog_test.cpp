#include "cinventoryexdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(InventoryExDialog, AddsQueriesAndCountsSlots){cInventoryExDialog d;EXPECT_EQ(d.GetBlankNum(),60u);EXPECT_TRUE(d.AddItem(100,20));EXPECT_TRUE(d.IsExist(0));EXPECT_EQ(d.GetItemForPos(0)->item_id,100);EXPECT_EQ(d.GetBlankNum(),59u);}
TEST(InventoryExDialog, MovesAndDeletesItems){cInventoryExDialog d;d.AddItem(100);d.AddItem(200);EXPECT_TRUE(d.MoveItem(0,5));EXPECT_FALSE(d.IsExist(0));EXPECT_EQ(d.GetItemForPos(5)->item_id,100);EXPECT_TRUE(d.DeleteItem(5));EXPECT_FALSE(d.IsExist(5));}
TEST(InventoryExDialog, LockedItemCannotMove){cInventoryExDialog d;d.AddItem(100);ASSERT_TRUE(d.SetItemLocked(0,true));EXPECT_FALSE(d.MoveItem(0,1));}
TEST(InventoryExDialog, DurabilityMoneyStateAndRelease){cInventoryExDialog d;d.AddItem(100,10);EXPECT_TRUE(d.UpdateItemDurabilityAdd(0,-3));EXPECT_EQ(d.GetItemForPos(0)->durability,7);d.SetMoney(99);d.SetState(InventoryState::Deal);d.ReleaseInventory();EXPECT_EQ(d.GetMoney(),0u);EXPECT_EQ(d.GetBlankNum(),60u);EXPECT_EQ(d.GetState(),InventoryState::Default);}

