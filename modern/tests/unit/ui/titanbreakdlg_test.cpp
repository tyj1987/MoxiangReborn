#include "titanbreakdlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(TitanBreakDlg, SetAndConfirmInvokesCallback) {
    cTitanBreakDlg d;
    TitanBreakState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetTitanBreakCallback([&](const TitanBreakState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(TitanBreakDlg, ClearRejectsDoubleConfirm) {
    cTitanBreakDlg d;
    TitanBreakState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetTitanBreakCallback([](const TitanBreakState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
