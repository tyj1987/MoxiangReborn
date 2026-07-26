#include "partywardialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(PartyWarDialog, SetAndConfirmInvokesCallback) {
    cPartyWarDialog d;
    PartyWarRequestState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetPartyWarRequestCallback([&](const PartyWarRequestState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(PartyWarDialog, ClearRejectsDoubleConfirm) {
    cPartyWarDialog d;
    PartyWarRequestState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetPartyWarRequestCallback([](const PartyWarRequestState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
