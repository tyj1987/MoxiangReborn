#include "disslovedlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(DissloveDlg, SetAndConfirmInvokesCallback) {
    cDissloveDlg d;
    DissloveRequestState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetDissloveRequestCallback([&](const DissloveRequestState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(DissloveDlg, ClearRejectsDoubleConfirm) {
    cDissloveDlg d;
    DissloveRequestState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetDissloveRequestCallback([](const DissloveRequestState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
