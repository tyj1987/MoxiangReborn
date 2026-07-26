#include "buyregdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(BuyRegDialog, SetAndConfirmInvokesCallback) {
    cBuyRegDialog d;
    BuyRegState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetBuyRegCallback([&](const BuyRegState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(BuyRegDialog, ClearRejectsDoubleConfirm) {
    cBuyRegDialog d;
    BuyRegState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetBuyRegCallback([](const BuyRegState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
