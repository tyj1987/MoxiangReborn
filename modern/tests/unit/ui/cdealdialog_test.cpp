#include "cdealdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(DealDialog, AddsItemsAndCompletesOnce){cDealDialog d;EXPECT_TRUE(d.AddOwnItem({100,2}));EXPECT_TRUE(d.AddOtherItem({200,1}));d.SetOwnMoney(50);d.SetOtherMoney(80);bool called=false;d.SetCompleteCallback([&](const auto&a,const auto&b,std::uint32_t net){called=true;return a.size()==1&&b.size()==1&&net==30;});EXPECT_TRUE(d.Confirm());EXPECT_TRUE(called);EXPECT_TRUE(d.IsConfirmed());EXPECT_FALSE(d.AddOwnItem({300,1}));EXPECT_FALSE(d.Confirm());}
TEST(DealDialog, CallbackFailureLeavesDealUnconfirmed){cDealDialog d;d.AddOwnItem({100,1});d.SetCompleteCallback([](const auto&,const auto&,std::uint32_t){return false;});EXPECT_FALSE(d.Confirm());EXPECT_FALSE(d.IsConfirmed());EXPECT_TRUE(d.AddOtherItem({200,1}));}
TEST(DealDialog, CancelRollsBackPendingDeal){cDealDialog d;d.AddOwnItem({100,1});d.SetOwnMoney(10);d.SetOtherMoney(20);d.Cancel();EXPECT_TRUE(d.IsCancelled());EXPECT_TRUE(d.OwnItems().empty());EXPECT_TRUE(d.OtherItems().empty());EXPECT_EQ(d.NetMoney(),0u);EXPECT_FALSE(d.AddOwnItem({200,1}));}
