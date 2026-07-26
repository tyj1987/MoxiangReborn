#include "suryundialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(SuryunDialog, SetAndConfirmInvokesCallback) {
    cSuryunDialog d;
    SuryunRequestState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetSuryunRequestCallback([&](const SuryunRequestState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(SuryunDialog, ClearRejectsDoubleConfirm) {
    cSuryunDialog d;
    SuryunRequestState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetSuryunRequestCallback([](const SuryunRequestState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
