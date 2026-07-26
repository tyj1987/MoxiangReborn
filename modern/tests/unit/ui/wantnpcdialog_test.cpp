#include "wantnpcdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(WantNpcDialog, SetAndConfirmInvokesCallback) {
    cWantNpcDialog d;
    WantNpcPostState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetWantNpcPostCallback([&](const WantNpcPostState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(WantNpcDialog, ClearRejectsDoubleConfirm) {
    cWantNpcDialog d;
    WantNpcPostState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetWantNpcPostCallback([](const WantNpcPostState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
