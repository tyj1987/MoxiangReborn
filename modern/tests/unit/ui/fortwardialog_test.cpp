#include "fortwardialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(FortWarDialog, SetAndConfirmInvokesCallback) {
    cFortWarDialog d;
    FortWarRequestState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetFortWarRequestCallback([&](const FortWarRequestState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(FortWarDialog, ClearRejectsDoubleConfirm) {
    cFortWarDialog d;
    FortWarRequestState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetFortWarRequestCallback([](const FortWarRequestState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
