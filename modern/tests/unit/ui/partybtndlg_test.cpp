#include "partybtndlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(PartyBtnDlg, SetAndConfirmInvokesCallback) {
    cPartyBtnDlg d;
    PartyBtnStateState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetPartyBtnStateCallback([&](const PartyBtnStateState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(PartyBtnDlg, ClearRejectsDoubleConfirm) {
    cPartyBtnDlg d;
    PartyBtnStateState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetPartyBtnStateCallback([](const PartyBtnStateState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
