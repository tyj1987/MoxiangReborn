#include "cjackpotdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(cJackpotDialog, SetAndConfirmInvokesCallback) {
    ccJackpotDialog d;
    JackpotJoinState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetJackpotJoinCallback([&](const JackpotJoinState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(cJackpotDialog, ClearRejectsDoubleConfirm) {
    ccJackpotDialog d;
    JackpotJoinState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetJackpotJoinCallback([](const JackpotJoinState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());  // double-confirm rejected
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
