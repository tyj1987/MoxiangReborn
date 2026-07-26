#include "mixdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(MixDialog, SetAndConfirmInvokesCallback) {
    cMixDialog d;
    MixRecipeState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetMixRecipeCallback([&](const MixRecipeState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(MixDialog, ClearRejectsDoubleConfirm) {
    cMixDialog d;
    MixRecipeState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetMixRecipeCallback([](const MixRecipeState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());  // double-confirm rejected
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
