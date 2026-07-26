#include "mnjoindialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(MNJoinDialog, SetAndConfirmInvokesCallback) {
    cMNJoinDialog d;
    MNJoinRoomState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetMNJoinRoomCallback([&](const MNJoinRoomState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(MNJoinDialog, ClearRejectsDoubleConfirm) {
    cMNJoinDialog d;
    MNJoinRoomState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetMNJoinRoomCallback([](const MNJoinRoomState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
