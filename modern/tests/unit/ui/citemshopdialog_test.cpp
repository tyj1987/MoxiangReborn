#include "citemshopdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(ItemShopDialog, CalculatesAndCompletesPurchase){cItemShopDialog d;d.SetEntries({{101,25,1}});d.SetMoney(100);ShopEntry got{};std::uint16_t qty=0;d.SetPurchaseCallback([&](const ShopEntry&e,std::uint16_t q){got=e;qty=q;return true;});EXPECT_EQ(d.TotalPrice(0,3),75u);EXPECT_TRUE(d.Buy(0,3));EXPECT_EQ(d.GetMoney(),25u);EXPECT_EQ(got.item_id,101);EXPECT_EQ(qty,3);}
TEST(ItemShopDialog, RejectsInsufficientFundsAndInvalidRows){cItemShopDialog d;d.SetEntries({{101,25,1}});d.SetMoney(20);EXPECT_FALSE(d.Buy(0));EXPECT_EQ(d.GetMoney(),20u);EXPECT_FALSE(d.Buy(4));EXPECT_EQ(d.TotalPrice(4),0u);}
TEST(ItemShopDialog, CallbackFailureDoesNotCharge){cItemShopDialog d;d.SetEntries({{101,25,1}});d.SetMoney(50);d.SetPurchaseCallback([](const ShopEntry&,std::uint16_t){return false;});EXPECT_FALSE(d.Buy(0));EXPECT_EQ(d.GetMoney(),50u);}
