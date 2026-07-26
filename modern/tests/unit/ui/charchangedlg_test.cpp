#include "charchangedlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(CharChangeDlg, SetAndConfirmInvokesCallback) {
    cCharChangeDlg d;
    CharSlotPickState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetCharSlotPickCallback([&](const CharSlotPickState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(CharChangeDlg, ClearRejectsDoubleConfirm) {
    cCharChangeDlg d;
    CharSlotPickState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetCharSlotPickCallback([](const CharSlotPickState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());  // double-confirm rejected
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
