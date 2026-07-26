#include "titanregisterdlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(TitanRegisterDlg, SetAndConfirmInvokesCallback) {
    cTitanRegisterDlg d;
    TitanRegisterState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetTitanRegisterCallback([&](const TitanRegisterState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(TitanRegisterDlg, ClearRejectsDoubleConfirm) {
    cTitanRegisterDlg d;
    TitanRegisterState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetTitanRegisterCallback([](const TitanRegisterState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
