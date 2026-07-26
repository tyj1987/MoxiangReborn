#include "serverlistdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(ServerListDialog, SetAndConfirmInvokesCallback) {
    cServerListDialog d;
    ServerSelectState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetServerSelectCallback([&](const ServerSelectState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(ServerListDialog, ClearRejectsDoubleConfirm) {
    cServerListDialog d;
    ServerSelectState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetServerSelectCallback([](const ServerSelectState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());  // double-confirm rejected
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
