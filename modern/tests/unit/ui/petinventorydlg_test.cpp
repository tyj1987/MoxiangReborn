#include "petinventorydlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(PetInventoryDlg, SetAndConfirmInvokesCallback) {
    cPetInventoryDlg d;
    PetInventoryRefreshState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetPetInventoryRefreshCallback([&](const PetInventoryRefreshState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(PetInventoryDlg, ClearRejectsDoubleConfirm) {
    cPetInventoryDlg d;
    PetInventoryRefreshState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetPetInventoryRefreshCallback([](const PetInventoryRefreshState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
