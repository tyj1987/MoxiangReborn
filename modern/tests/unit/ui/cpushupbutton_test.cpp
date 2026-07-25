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

TEST(CPushupButton, NonPassiveClickTogglesPushed) {
    // 1:1 with legacy cPushupButton click behaviour: a non-passive
    // pushup button toggles its pushed state on every click inside
    // its bounds.  The toggle is the "sticky" pattern used for
    // dialog tabs (Guild's Member/Skill/Info etc.).
    //
    // cButton's state machine requires two ActionEvent calls per
    // click (LButtonDown + LButtonUp) for the click to fire, so we
    // simulate that pair.
    mxh::ui::cPushupButton b;
    b.Init(0, 0, 40, 20, nullptr, nullptr, nullptr,
           mxh::ui::cButton::ClickCallback{}, nullptr, 42);
    EXPECT_FALSE(b.IsPushed());

    auto click = [&b](std::int32_t x, std::int32_t y) {
        b.ActionEvent(x, y, mxh::ui::cWindow::MouseFlagLButton);  // down
        b.ActionEvent(x, y, 0);                                    // up
    };

    // First click: pushed = true.
    click(20, 10);
    EXPECT_TRUE(b.IsPushed());

    // Second click: pushed = false.
    click(20, 10);
    EXPECT_FALSE(b.IsPushed());

    // Third click: pushed = true again.
    click(20, 10);
    EXPECT_TRUE(b.IsPushed());
}

TEST(CPushupButton, ClickOutsideBoundsDoesNotToggle) {
    // 1:1 with cButton::ActionEvent: a click outside the button
    // bounds must not flip the pushed state.  (cPushupButton
    // inherits the bound-check from cButton.)
    mxh::ui::cPushupButton b;
    b.Init(0, 0, 40, 20, nullptr, nullptr, nullptr,
           mxh::ui::cButton::ClickCallback{}, nullptr, 42);
    // Click at (100, 100) — outside the 40x20 bounds starting at (0, 0).
    b.ActionEvent(100, 100, mxh::ui::cWindow::MouseFlagLButton);
    EXPECT_FALSE(b.IsPushed());
}
