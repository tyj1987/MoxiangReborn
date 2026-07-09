// cpushupbutton_test.cpp — Phase 6.12 coverage for cPushupButton (toggle).

#include "cPushupButton.hpp"

#include <gtest/gtest.h>

TEST(CPushupButton, DefaultStateIsNotPushedAndNotPassive) {
    mxh::ui::cPushupButton b;
    EXPECT_FALSE(b.IsPushed());
    EXPECT_FALSE(b.IsPassive());
}

TEST(CPushupButton, SetPushTogglesFlag) {
    mxh::ui::cPushupButton b;
    b.SetPush(true);
    EXPECT_TRUE(b.IsPushed());
    b.SetPush(false);
    EXPECT_FALSE(b.IsPushed());
}

TEST(CPushupButton, SetPushExIsAliasForSetPush) {
    mxh::ui::cPushupButton b;
    b.SetPushEx(true);
    EXPECT_TRUE(b.IsPushed());
    b.SetPushEx(false);
    EXPECT_FALSE(b.IsPushed());
}

TEST(CPushupButton, PassiveSetterGetter) {
    mxh::ui::cPushupButton b;
    b.SetPassive(true);
    EXPECT_TRUE(b.IsPassive());
    b.SetPassive(false);
    EXPECT_FALSE(b.IsPassive());
}

TEST(CPushupButton, PassiveClickReturnsNull) {
    mxh::ui::cPushupButton b;
    b.Init(0, 0, 40, 20, nullptr, nullptr, nullptr,
           mxh::ui::cButton::ClickCallback{}, nullptr, 42);
    b.SetPassive(true);
    // Click at button center — passive buttons ignore.
    const std::uint32_t we = b.ActionEvent(20, 10, mxh::ui::cWindow::MouseFlagLButton);
    EXPECT_EQ(we, static_cast<std::uint32_t>(mxh::ui::cWindow::WindowEvent::Null));
    EXPECT_FALSE(b.IsPushed());  // did not toggle
}
