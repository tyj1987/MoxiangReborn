#include "monsterguagedlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(MonsterGuageDlg, SetAndConfirmInvokesCallback) {
    cMonsterGuageDlg d;
    MonsterGuageRefreshState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetMonsterGuageRefreshCallback([&](const MonsterGuageRefreshState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(MonsterGuageDlg, ClearRejectsDoubleConfirm) {
    cMonsterGuageDlg d;
    MonsterGuageRefreshState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetMonsterGuageRefreshCallback([](const MonsterGuageRefreshState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
