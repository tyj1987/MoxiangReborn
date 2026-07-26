#include "uniqueitemcursecancellationdlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(UniqueItemCurseCancellationDlg, SetAndConfirmInvokesCallback) {
    cUniqueItemCurseCancellationDlg d;
    UniqueItemCurseCancelState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetUniqueItemCurseCancelCallback([&](const UniqueItemCurseCancelState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(UniqueItemCurseCancellationDlg, ClearRejectsDoubleConfirm) {
    cUniqueItemCurseCancellationDlg d;
    UniqueItemCurseCancelState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetUniqueItemCurseCancelCallback([](const UniqueItemCurseCancelState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
