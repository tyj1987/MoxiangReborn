#include "titandissolutiondlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(TitanDissolutionDlg, SetAndConfirmInvokesCallback) {
    cTitanDissolutionDlg d;
    TitanDissolutionState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetTitanDissolutionCallback([&](const TitanDissolutionState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(TitanDissolutionDlg, ClearRejectsDoubleConfirm) {
    cTitanDissolutionDlg d;
    TitanDissolutionState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetTitanDissolutionCallback([](const TitanDissolutionState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
