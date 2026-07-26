#include "guildtraineedialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(GuildTraineeDialog, SetAndConfirmInvokesCallback) {
    cGuildTraineeDialog d;
    GuildTraineeState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetGuildTraineeCallback([&](const GuildTraineeState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(GuildTraineeDialog, ClearRejectsDoubleConfirm) {
    cGuildTraineeDialog d;
    GuildTraineeState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetGuildTraineeCallback([](const GuildTraineeState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
