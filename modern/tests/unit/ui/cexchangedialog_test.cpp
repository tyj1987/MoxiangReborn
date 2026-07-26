#include "cexchangedialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(ExchangeDialog, RequiresBothSidesToConfirm){cExchangeDialog d;EXPECT_TRUE(d.SetOwn(0,{1,2}));EXPECT_TRUE(d.SetOther(0,{2,1}));EXPECT_TRUE(d.SetOwnConfirmed(true));EXPECT_FALSE(d.CanComplete());EXPECT_TRUE(d.SetOtherConfirmed(true));EXPECT_TRUE(d.CanComplete());EXPECT_TRUE(d.Complete());}
TEST(ExchangeDialog, ChangingItemsInvalidatesConfirmation){cExchangeDialog d;d.SetOwn(0,{1,1});d.SetOther(0,{2,1});EXPECT_TRUE(d.SetOwn(1,{3,1}));EXPECT_TRUE(d.SetOwnConfirmed(true));EXPECT_TRUE(d.SetOtherConfirmed(true));EXPECT_TRUE(d.CanComplete());}
TEST(ExchangeDialog, CancelClearsBothSides){cExchangeDialog d;d.SetOwn(0,{1,1});d.SetOther(0,{2,1});d.Cancel();EXPECT_TRUE(d.IsCancelled());EXPECT_EQ(d.Own()[0].item_id,0);EXPECT_EQ(d.Other()[0].item_id,0);EXPECT_FALSE(d.SetOwn(1,{3,1}));}

