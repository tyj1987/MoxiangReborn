#include "cguildwarehousedialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(GuildWarehouseDialog, StoresTakesAndMovesItems){cGuildWarehouseDialog d;d.SetPermission(true,true);EXPECT_TRUE(d.Store(0,{1,5}));EXPECT_TRUE(d.Move(0,3));auto x=d.Take(3);ASSERT_TRUE(x);EXPECT_EQ(x->item_id,1);}
TEST(GuildWarehouseDialog, EnforcesPermissionsAndLock){cGuildWarehouseDialog d;d.SetPermission(true,false);EXPECT_TRUE(d.Store(0,{1,1}));EXPECT_FALSE(d.Take(0));d.SetLocked(true);EXPECT_FALSE(d.Store(1,{2,1}));}
TEST(GuildWarehouseDialog, RejectsOccupiedAndInvalidSlots){cGuildWarehouseDialog d;d.SetPermission(true,true);EXPECT_TRUE(d.Store(0,{1,1}));EXPECT_FALSE(d.Store(0,{2,1}));EXPECT_FALSE(d.Store(60,{2,1}));EXPECT_FALSE(d.Store(1,{0,1}));}
