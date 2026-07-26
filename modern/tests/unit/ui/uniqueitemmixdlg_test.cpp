#include "uniqueitemmixdlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(UniqueItemMixDlg, SetAndConfirmInvokesCallback) {
    cUniqueItemMixDlg d;
    UniqueItemMixState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetUniqueItemMixCallback([&](const UniqueItemMixState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(UniqueItemMixDlg, ClearRejectsDoubleConfirm) {
    cUniqueItemMixDlg d;
    UniqueItemMixState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetUniqueItemMixCallback([](const UniqueItemMixState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
