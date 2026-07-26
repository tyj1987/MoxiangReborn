#include "guildfieldwardialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(GuildFieldWarDialog, SetAndConfirmInvokesCallback) {
    cGuildFieldWarDialog d;
    GuildFieldWarRequestState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetGuildFieldWarRequestCallback([&](const GuildFieldWarRequestState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(GuildFieldWarDialog, ClearRejectsDoubleConfirm) {
    cGuildFieldWarDialog d;
    GuildFieldWarRequestState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetGuildFieldWarRequestCallback([](const GuildFieldWarRequestState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
