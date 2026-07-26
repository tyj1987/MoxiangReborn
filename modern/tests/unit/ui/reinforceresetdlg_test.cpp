#include "reinforceresetdlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(ReinforceResetDlg, SetAndConfirmInvokesCallback) {
    cReinforceResetDlg d;
    ReinforceResetState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetReinforceResetCallback([&](const ReinforceResetState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(ReinforceResetDlg, ClearRejectsDoubleConfirm) {
    cReinforceResetDlg d;
    ReinforceResetState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetReinforceResetCallback([](const ReinforceResetState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());  // double-confirm rejected
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
