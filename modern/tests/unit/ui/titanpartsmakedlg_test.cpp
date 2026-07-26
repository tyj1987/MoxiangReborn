#include "titanpartsmakedlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(TitanPartsMakeDlg, SetAndConfirmInvokesCallback) {
    cTitanPartsMakeDlg d;
    TitanPartsMakeState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetTitanPartsMakeCallback([&](const TitanPartsMakeState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(TitanPartsMakeDlg, ClearRejectsDoubleConfirm) {
    cTitanPartsMakeDlg d;
    TitanPartsMakeState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetTitanPartsMakeCallback([](const TitanPartsMakeState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
