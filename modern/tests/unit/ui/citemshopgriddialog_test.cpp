#include "citemshopgriddialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(ItemShopGridDialog, PaginatesAndSelectsCells){cItemShopGridDialog d;std::vector<ShopGridItem> items(21);items[20]={21,99,2};d.SetItems(items);EXPECT_EQ(d.PageCount(),2u);d.SelectPage(1);EXPECT_TRUE(d.Select(0));EXPECT_EQ(d.Selected()->id,21u);}
TEST(ItemShopGridDialog, RejectsOutOfRangeCellsAndPages){cItemShopGridDialog d;d.SetItems({{1,1,1}});d.SelectPage(2);EXPECT_FALSE(d.Select(1));EXPECT_TRUE(d.Select(0));}
TEST(ItemShopGridDialog, ChecksStockForPurchase){cItemShopGridDialog d;d.SetItems({{1,10,2}});d.Select(0);EXPECT_TRUE(d.CanBuy(2));EXPECT_FALSE(d.CanBuy(3));EXPECT_FALSE(d.CanBuy(0));}

