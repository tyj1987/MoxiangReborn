#include "guildrankdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(GuildRankDialog, SetAndConfirmInvokesCallback) {
    cGuildRankDialog d;
    GuildRankUpdateState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetGuildRankUpdateCallback([&](const GuildRankUpdateState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(GuildRankDialog, ClearRejectsDoubleConfirm) {
    cGuildRankDialog d;
    GuildRankUpdateState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetGuildRankUpdateCallback([](const GuildRankUpdateState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
