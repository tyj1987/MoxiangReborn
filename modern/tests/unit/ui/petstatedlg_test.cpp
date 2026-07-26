#include "petstatedlg.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(PetStateDlg, SetAndConfirmInvokesCallback) {
    cPetStateDlg d;
    PetStateRefreshState s{};
    EXPECT_TRUE(d.Set(s));
    bool called = false;
    d.SetPetStateRefreshCallback([&](const PetStateRefreshState&) { called = true; return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_TRUE(called);
    EXPECT_TRUE(d.IsConfirmed());
}
TEST(PetStateDlg, ClearRejectsDoubleConfirm) {
    cPetStateDlg d;
    PetStateRefreshState s{};
    EXPECT_TRUE(d.Set(s));
    d.SetPetStateRefreshCallback([](const PetStateRefreshState&) { return true; });
    EXPECT_TRUE(d.Confirm());
    EXPECT_FALSE(d.Confirm());
    d.Clear();
    EXPECT_FALSE(d.IsConfirmed());
}
