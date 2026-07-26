#include "skillpointresetdlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(SkillPointResetDlg, SetAndConfirmInvokesCallback) {
    cSkillPointResetDlg d;
    SkillPointResetState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetSkillPointResetCallback([&](const SkillPointResetState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(SkillPointResetDlg, ClearRejectsDoubleConfirm) {
    cSkillPointResetDlg d;
    SkillPointResetState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetSkillPointResetCallback([](const SkillPointResetState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
