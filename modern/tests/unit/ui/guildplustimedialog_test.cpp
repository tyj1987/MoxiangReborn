#include "guildplustimedialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(GuildPlusTimeDialog, SetAndConfirmInvokesCallback) {
    cGuildPlusTimeDialog d;
    GuildPlusTimeState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetGuildPlusTimeCallback([&](const GuildPlusTimeState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(GuildPlusTimeDialog, ClearRejectsDoubleConfirm) {
    cGuildPlusTimeDialog d;
    GuildPlusTimeState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetGuildPlusTimeCallback([](const GuildPlusTimeState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
