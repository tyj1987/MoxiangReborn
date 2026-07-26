#include "privatewarehousedialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(PrivateWarehouseDialog, SetAndConfirmInvokesCallback) {
    cPrivateWarehouseDialog d;
    PrivateWarehouseState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetPrivateWarehouseCallback([&](const PrivateWarehouseState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(PrivateWarehouseDialog, ClearRejectsDoubleConfirm) {
    cPrivateWarehouseDialog d;
    PrivateWarehouseState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetPrivateWarehouseCallback([](const PrivateWarehouseState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
