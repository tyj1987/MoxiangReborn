#include "upgradedlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(UpgradeDlg, SetAndConfirmInvokesCallback) {
    cUpgradeDlg d;
    UpgradeRequestState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetUpgradeRequestCallback([&](const UpgradeRequestState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(UpgradeDlg, ClearRejectsDoubleConfirm) {
    cUpgradeDlg d;
    UpgradeRequestState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetUpgradeRequestCallback([](const UpgradeRequestState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());  // double-confirm rejected
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
