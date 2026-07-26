#include "petupgradedialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(PetUpgradeDialog, SetAndConfirmInvokesCallback) {
    cPetUpgradeDialog d;
    PetUpgradeRequestState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetPetUpgradeRequestCallback([&](const PetUpgradeRequestState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(PetUpgradeDialog, ClearRejectsDoubleConfirm) {
    cPetUpgradeDialog d;
    PetUpgradeRequestState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetPetUpgradeRequestCallback([](const PetUpgradeRequestState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
