#include "partycreatedlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(PartyCreateDlg, SetAndConfirmInvokesCallback) {
    cPartyCreateDlg d;
    PartyCreateRequestState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetPartyCreateRequestCallback([&](const PartyCreateRequestState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(PartyCreateDlg, ClearRejectsDoubleConfirm) {
    cPartyCreateDlg d;
    PartyCreateRequestState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetPartyCreateRequestCallback([](const PartyCreateRequestState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
