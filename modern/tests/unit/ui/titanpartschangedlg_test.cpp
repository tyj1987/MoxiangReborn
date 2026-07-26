#include "titanpartschangedlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(TitanPartsChangeDlg, SetAndConfirmInvokesCallback) {
    cTitanPartsChangeDlg d;
    TitanPartsChangeState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetTitanPartsChangeCallback([&](const TitanPartsChangeState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(TitanPartsChangeDlg, ClearRejectsDoubleConfirm) {
    cTitanPartsChangeDlg d;
    TitanPartsChangeState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetTitanPartsChangeCallback([](const TitanPartsChangeState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
