#include "minimapdlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(MiniMapDlg, SetAndConfirmInvokesCallback) {
    cMiniMapDlg d;
    MiniMapTickState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetMiniMapTickCallback([&](const MiniMapTickState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(MiniMapDlg, ClearRejectsDoubleConfirm) {
    cMiniMapDlg d;
    MiniMapTickState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetMiniMapTickCallback([](const MiniMapTickState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());  // double-confirm rejected
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
