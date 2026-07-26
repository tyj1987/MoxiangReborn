#include "petrevivaldialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(PetRevivalDialog, SetAndConfirmInvokesCallback) {
    cPetRevivalDialog d;
    PetRevivalRequestState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetPetRevivalRequestCallback([&](const PetRevivalRequestState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(PetRevivalDialog, ClearRejectsDoubleConfirm) {
    cPetRevivalDialog d;
    PetRevivalRequestState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetPetRevivalRequestCallback([](const PetRevivalRequestState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
