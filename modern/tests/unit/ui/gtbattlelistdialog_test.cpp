#include "gtbattlelistdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(GTBattleListDialog, SetAndConfirmInvokesCallback) {
    cGTBattleListDialog d;
    GTBattleListRefreshState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetGTBattleListRefreshCallback([&](const GTBattleListRefreshState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(GTBattleListDialog, ClearRejectsDoubleConfirm) {
    cGTBattleListDialog d;
    GTBattleListRefreshState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetGTBattleListRefreshCallback([](const GTBattleListRefreshState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
