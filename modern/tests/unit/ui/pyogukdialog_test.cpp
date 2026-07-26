#include "pyogukdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(PyoGukDialog, SetAndConfirmInvokesCallback) {
    cPyoGukDialog d;
    WarehouseState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetWarehouseCallback([&](const WarehouseState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(PyoGukDialog, ClearRejectsDoubleConfirm) {
    cPyoGukDialog d;
    WarehouseState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetWarehouseCallback([](const WarehouseState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());  // double-confirm rejected
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
