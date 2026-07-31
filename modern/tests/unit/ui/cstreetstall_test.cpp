#include "mxh/ui/cstreetstall.hpp"
#include "mxh/ui/cdialog.hpp"
#include "mxh/ui/cwindow.hpp"
#include <gtest/gtest.h>
#include <type_traits>
using mxh::ui::cDialog; using mxh::ui::cStreetStall; using mxh::ui::cWindow; using mxh::ui::StallDlgState; using mxh::ui::StallOption;

TEST(CStreetStallTest, SurfaceAndDefaults) { static_assert(std::is_base_of_v<cDialog,cStreetStall>); static_assert(std::is_base_of_v<cWindow,cStreetStall>); static_assert(!std::is_copy_constructible_v<cStreetStall>); cStreetStall d; EXPECT_EQ(d.GetDlgState(),StallDlgState::NotOpened); EXPECT_EQ(d.GetCurSelectedItemNum(),-1); EXPECT_EQ(d.GetStallOwnerId(),0u); EXPECT_EQ(d.GetData(),nullptr); EXPECT_EQ(cStreetStall::kStallSlotCount,25u); EXPECT_EQ(cStreetStall::kTitleMaxLen,66u); }
TEST(CStreetStallTest, InitAndOwnerData) { cStreetStall d; d.Init(4,7,320,240,nullptr,cStreetStall::kIdDialog); EXPECT_EQ(d.absX(),4); EXPECT_EQ(d.absY(),7); EXPECT_EQ(d.width(),320u); EXPECT_EQ(d.height(),240u); EXPECT_EQ(d.id(),cStreetStall::kIdDialog); int value=1; d.SetStallOwnerId(1234); d.SetData(&value); EXPECT_EQ(d.GetStallOwnerId(),1234u); EXPECT_EQ(d.GetData(),&value); d.ResetDlgData(); EXPECT_EQ(d.GetData(),nullptr); }
TEST(CStreetStallTest, StatesAndClose) { cStreetStall d; d.ShowSellStall(); EXPECT_EQ(d.GetDlgState(),StallDlgState::Opened); d.ShowBuyStall(); EXPECT_EQ(d.GetDlgState(),StallDlgState::Buy); d.OnCloseStall(true); EXPECT_EQ(d.GetDlgState(),StallDlgState::NotOpened); }
TEST(CStreetStallTest, MoneyAndSelection) { cStreetStall d; d.RegistMoney(3,777); EXPECT_EQ(d.GetItemMoney(3),777u); EXPECT_EQ(d.GetItemMoney(24),0u); d.SetCurSelectedItemNum(7); EXPECT_EQ(d.GetCurSelectedItemNum(),7); EXPECT_EQ(d.GetGridposForItemIdx(7),-1); EXPECT_EQ(d.GetGridposForItemIdx(25),-1); }
TEST(CStreetStallTest, TitleAndSafeOperations) { cStreetStall d; char input[]="stall title"; d.RegistTitle(input,true); char output[80]={}; d.GetTitle(output); EXPECT_STREQ(output,""); d.ResetItemInfo(100,2); d.DeleteItem(nullptr); d.DeleteItemAll(); EXPECT_EQ(d.ActionEvent(1,2,3),cStreetStall::kWeNull); EXPECT_FALSE(d.FakeMoveIcon(1,2,nullptr)); }
TEST(CStreetStallTest, DivideAndCheckCallbacks) { struct T { int calls=0; bool value=true; }; T t; cStreetStall d; d.SetSelectedItemCheckCallbackForTest(+[](void* p){auto* t=static_cast<T*>(p);++t->calls;return t->value;},&t); d.SetMoneyEditCheckCallbackForTest(+[](void* p){auto* t=static_cast<T*>(p);++t->calls;return t->value;},&t); EXPECT_TRUE(d.SelectedItemCheck()); EXPECT_TRUE(d.MoneyEditCheck()); EXPECT_EQ(t.calls,2); }
