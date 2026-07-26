#include "skilloptionchangedlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(SkillOptionChangeDlg, SetAndConfirmInvokesCallback) {
    cSkillOptionChangeDlg d;
    SkillOptionChangeState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetSkillOptionChangeCallback([&](const SkillOptionChangeState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(SkillOptionChangeDlg, ClearRejectsDoubleConfirm) {
    cSkillOptionChangeDlg d;
    SkillOptionChangeState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetSkillOptionChangeCallback([](const SkillOptionChangeState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
