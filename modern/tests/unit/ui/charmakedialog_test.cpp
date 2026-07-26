#include "charmakedialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(CharMakeDialog, SetAndConfirmInvokesCallback) {
    cCharMakeDialog d;
    CharMakeSubmitState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetCharMakeSubmitCallback([&](const CharMakeSubmitState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(CharMakeDialog, ClearRejectsDoubleConfirm) {
    cCharMakeDialog d;
    CharMakeSubmitState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetCharMakeSubmitCallback([](const CharMakeSubmitState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
