#include "bigmapdlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(BigMapDlg, SetAndConfirmInvokesCallback) {
    cBigMapDlg d;
    BigMapClickState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetBigMapClickCallback([&](const BigMapClickState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(BigMapDlg, ClearRejectsDoubleConfirm) {
    cBigMapDlg d;
    BigMapClickState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetBigMapClickCallback([](const BigMapClickState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());  // double-confirm rejected
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
