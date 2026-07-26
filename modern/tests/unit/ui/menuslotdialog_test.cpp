#include "menuslotdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(MenuSlotDialog, SetAndConfirmInvokesCallback) {
    cMenuSlotDialog d;
    MenuSlotAssignState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetMenuSlotAssignCallback([&](const MenuSlotAssignState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(MenuSlotDialog, ClearRejectsDoubleConfirm) {
    cMenuSlotDialog d;
    MenuSlotAssignState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetMenuSlotAssignCallback([](const MenuSlotAssignState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());  // double-confirm rejected
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
