#include "partymatchingdlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(PartyMatchingDlg, SetAndConfirmInvokesCallback) {
    cPartyMatchingDlg d;
    PartyMatchingState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetPartyMatchingCallback([&](const PartyMatchingState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(PartyMatchingDlg, ClearRejectsDoubleConfirm) {
    cPartyMatchingDlg d;
    PartyMatchingState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetPartyMatchingCallback([](const PartyMatchingState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
