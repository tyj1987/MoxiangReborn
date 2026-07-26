#include "mncreatedialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(MNCreateDialog, SetAndConfirmInvokesCallback) {
    cMNCreateDialog d;
    MNCreateRoomState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetMNCreateRoomCallback([&](const MNCreateRoomState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(MNCreateDialog, ClearRejectsDoubleConfirm) {
    cMNCreateDialog d;
    MNCreateRoomState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetMNCreateRoomCallback([](const MNCreateRoomState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
