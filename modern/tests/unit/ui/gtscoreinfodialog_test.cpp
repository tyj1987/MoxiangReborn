#include "gtscoreinfodialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(GTScoreInfoDialog, SetAndConfirmInvokesCallback) {
    cGTScoreInfoDialog d;
    GTScoreInfoRefreshState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetGTScoreInfoRefreshCallback([&](const GTScoreInfoRefreshState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(GTScoreInfoDialog, ClearRejectsDoubleConfirm) {
    cGTScoreInfoDialog d;
    GTScoreInfoRefreshState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetGTScoreInfoRefreshCallback([](const GTScoreInfoRefreshState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
