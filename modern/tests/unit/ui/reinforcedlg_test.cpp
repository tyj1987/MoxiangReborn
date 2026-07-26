#include "reinforcedlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(ReinforceDlg, SetAndConfirmInvokesCallback) {
    cReinforceDlg d;
    ReinforceState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetReinforceCallback([&](const ReinforceState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(ReinforceDlg, ClearRejectsDoubleConfirm) {
    cReinforceDlg d;
    ReinforceState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetReinforceCallback([](const ReinforceState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());  // double-confirm rejected
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
