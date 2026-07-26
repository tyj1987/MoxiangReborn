#include "cdissolutiondialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(DissolutionDialog, PreviewsAndConfirmsItem){cDissolutionDialog d;EXPECT_TRUE(d.SetItem({10,20,3}));DissolutionItem got{};d.SetConfirmCallback([&](auto&i){got=i;return true;});EXPECT_TRUE(d.Confirm());EXPECT_TRUE(d.IsConfirmed());EXPECT_EQ(got.material_count,3);}
TEST(DissolutionDialog, InvalidItemAndDoubleConfirmRejected){cDissolutionDialog d;EXPECT_FALSE(d.SetItem({0,20,1}));EXPECT_FALSE(d.Confirm());d.SetItem({1,2,1});EXPECT_TRUE(d.Confirm());EXPECT_FALSE(d.Confirm());}
TEST(DissolutionDialog, CallbackFailureLeavesPendingState){cDissolutionDialog d;d.SetItem({1,2,1});d.SetConfirmCallback([](auto&){return false;});EXPECT_FALSE(d.Confirm());EXPECT_FALSE(d.IsConfirmed());d.ClearItem();EXPECT_FALSE(d.Item().has_value());}
