#include "mxh/ui/cstreetbuystall.hpp"
#include <gtest/gtest.h>
#include <type_traits>
using mxh::ui::cDialog; using mxh::ui::cStreetBuyStall; using mxh::ui::BuyStallDlgState;
TEST(CStreetBuyStallTest, SurfaceAndDefaults){static_assert(std::is_base_of_v<cDialog,cStreetBuyStall>);static_assert(!std::is_copy_constructible_v<cStreetBuyStall>);cStreetBuyStall d;EXPECT_EQ(d.GetDlgState(),BuyStallDlgState::NotOpened);EXPECT_EQ(d.GetCurSelectedItemNum(),-1);EXPECT_EQ(d.GetStallOwnerId(),0u);EXPECT_EQ(cStreetBuyStall::kStallSlotCount,5u);}
TEST(CStreetBuyStallTest, StatesAndClose){cStreetBuyStall d;d.ShowSellStall();EXPECT_EQ(d.GetDlgState(),BuyStallDlgState::Opened);d.ShowBuyStall();EXPECT_EQ(d.GetDlgState(),BuyStallDlgState::Sell);d.OnCloseStall(true);EXPECT_EQ(d.GetDlgState(),BuyStallDlgState::NotOpened);}
TEST(CStreetBuyStallTest, MoneyAndRegInfoBounds){cStreetBuyStall d;d.RegistMoney(2,777);EXPECT_EQ(d.GetItemMoney(2),777u);EXPECT_EQ(d.GetItemMoney(5),0u);d.ChangeItemStatus(2,4,900);EXPECT_EQ(d.GetBuyRegInfo(2).volume,4);EXPECT_EQ(d.GetBuyRegInfo(2).money,900u);}
TEST(CStreetBuyStallTest, TitleRoundTrip){cStreetBuyStall d;char in[]="buyer title";d.RegistTitle(in,true);char out[80]={};d.GetTitle(out);EXPECT_STREQ(out,"buyer title");}
TEST(CStreetBuyStallTest, CheckCallbacks){struct T{int n=0;};T t;cStreetBuyStall d;d.SetSelectedItemCheckCallbackForTest(+[](void*p){++static_cast<T*>(p)->n;return true;},&t);d.SetMoneyEditCheckCallbackForTest(+[](void*p){++static_cast<T*>(p)->n;return true;},&t);EXPECT_TRUE(d.SelectedItemCheck());EXPECT_TRUE(d.MoneyEditCheck());EXPECT_EQ(t.n,2);}
TEST(CStreetBuyStallTest, ResetClearsData){cStreetBuyStall d;int x=1;d.SetData(&x);d.SetStallOwnerId(8);d.ShowBuyStall();d.RegistMoney(0,2);d.ResetDlgData();EXPECT_EQ(d.GetData(),nullptr);EXPECT_EQ(d.GetStallOwnerId(),0u);EXPECT_EQ(d.GetDlgState(),BuyStallDlgState::NotOpened);EXPECT_EQ(d.GetItemMoney(0),0u);}

