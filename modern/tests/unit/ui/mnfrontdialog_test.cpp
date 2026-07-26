#include "mnfrontdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(MNFrontDialog, SetAndConfirmInvokesCallback) {
    cMNFrontDialog d;
    MNFrontRefreshState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetMNFrontRefreshCallback([&](const MNFrontRefreshState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(MNFrontDialog, ClearRejectsDoubleConfirm) {
    cMNFrontDialog d;
    MNFrontRefreshState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetMNFrontRefreshCallback([](const MNFrontRefreshState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
