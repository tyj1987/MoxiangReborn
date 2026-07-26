#include "questtotaldialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(QuestTotalDialog, SetAndConfirmInvokesCallback) {
    cQuestTotalDialog d;
    QuestTotalState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetQuestTotalCallback([&](const QuestTotalState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(QuestTotalDialog, ClearRejectsDoubleConfirm) {
    cQuestTotalDialog d;
    QuestTotalState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetQuestTotalCallback([](const QuestTotalState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());  // double-confirm rejected
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
