#include "titanmixdlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(TitanMixDlg, SetAndConfirmInvokesCallback) {
    cTitanMixDlg d;
    TitanMixState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetTitanMixCallback([&](const TitanMixState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(TitanMixDlg, ClearRejectsDoubleConfirm) {
    cTitanMixDlg d;
    TitanMixState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetTitanMixCallback([](const TitanMixState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
