#include "titanupgradedlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(TitanUpgradeDlg, SetAndConfirmInvokesCallback) {
    cTitanUpgradeDlg d;
    TitanUpgradeState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetTitanUpgradeCallback([&](const TitanUpgradeState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(TitanUpgradeDlg, ClearRejectsDoubleConfirm) {
    cTitanUpgradeDlg d;
    TitanUpgradeState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetTitanUpgradeCallback([](const TitanUpgradeState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
