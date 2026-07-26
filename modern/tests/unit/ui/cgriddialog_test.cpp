#include "cgriddialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(GridDialog, SetAndConfirmInvokesCallback) {
    cGridDialog d;
    GridClickState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetGridClickCallback([&](const GridClickState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(GridDialog, ClearRejectsDoubleConfirm) {
    cGridDialog d;
    GridClickState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetGridClickCallback([](const GridClickState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
