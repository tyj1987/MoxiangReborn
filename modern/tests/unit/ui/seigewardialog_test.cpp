#include "seigewardialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(SeigeWarDialog, SetAndConfirmInvokesCallback) {
    cSeigeWarDialog d;
    SeigeWarMatchState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetSeigeWarMatchCallback([&](const SeigeWarMatchState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(SeigeWarDialog, ClearRejectsDoubleConfirm) {
    cSeigeWarDialog d;
    SeigeWarMatchState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetSeigeWarMatchCallback([](const SeigeWarMatchState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
