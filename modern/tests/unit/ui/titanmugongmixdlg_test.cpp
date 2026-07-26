#include "titanmugongmixdlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(TitanMugongMixDlg, SetAndConfirmInvokesCallback) {
    cTitanMugongMixDlg d;
    TitanMugongMixState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetTitanMugongMixCallback([&](const TitanMugongMixState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(TitanMugongMixDlg, ClearRejectsDoubleConfirm) {
    cTitanMugongMixDlg d;
    TitanMugongMixState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetTitanMugongMixCallback([](const TitanMugongMixState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
