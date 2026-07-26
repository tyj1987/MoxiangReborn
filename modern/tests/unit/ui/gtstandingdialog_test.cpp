#include "gtstandingdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(GTStandingDialog, SetAndConfirmInvokesCallback) {
    cGTStandingDialog d;
    GTStandingRefreshState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetGTStandingRefreshCallback([&](const GTStandingRefreshState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(GTStandingDialog, ClearRejectsDoubleConfirm) {
    cGTStandingDialog d;
    GTStandingRefreshState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetGTStandingRefreshCallback([](const GTStandingRefreshState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
