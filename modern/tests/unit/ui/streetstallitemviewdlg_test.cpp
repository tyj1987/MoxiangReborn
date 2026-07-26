#include "streetstallitemviewdlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(StreetStallItemViewDlg, SetAndConfirmInvokesCallback) {
    cStreetStallItemViewDlg d;
    StreetStallItemViewState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetStreetStallItemViewCallback([&](const StreetStallItemViewState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(StreetStallItemViewDlg, ClearRejectsDoubleConfirm) {
    cStreetStallItemViewDlg d;
    StreetStallItemViewState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetStreetStallItemViewCallback([](const StreetStallItemViewState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
