#include "partymemberdlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(PartyMemberDlg, SetAndConfirmInvokesCallback) {
    cPartyMemberDlg d;
    PartyMemberRefreshState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetPartyMemberRefreshCallback([&](const PartyMemberRefreshState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(PartyMemberDlg, ClearRejectsDoubleConfirm) {
    cPartyMemberDlg d;
    PartyMemberRefreshState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetPartyMemberRefreshCallback([](const PartyMemberRefreshState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
