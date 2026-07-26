#include "titaninventorydlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(TitanInventoryDlg, SetAndConfirmInvokesCallback) {
    cTitanInventoryDlg d;
    TitanInventoryState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetTitanInventoryCallback([&](const TitanInventoryState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(TitanInventoryDlg, ClearRejectsDoubleConfirm) {
    cTitanInventoryDlg d;
    TitanInventoryState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetTitanInventoryCallback([](const TitanInventoryState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
