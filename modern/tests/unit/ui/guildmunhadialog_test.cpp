#include "guildmunhadialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(GuildMunhaDialog, SetAndConfirmInvokesCallback) {
    cGuildMunhaDialog d;
    GuildMunhaUpdateState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetGuildMunhaUpdateCallback([&](const GuildMunhaUpdateState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(GuildMunhaDialog, ClearRejectsDoubleConfirm) {
    cGuildMunhaDialog d;
    GuildMunhaUpdateState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetGuildMunhaUpdateCallback([](const GuildMunhaUpdateState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
