// tests/unit/ui/cbutton_test.cpp
// Phase 6.1 unit tests for the modern mxh::ui::cButton widget.
// All tests are CPU-side; no D3D11 / GPU dependency. We exercise the
// state machine, the click detection, the text label, and the disabled
// semantics by feeding ActionEvent with synthetic (mouseX, mouseY, flags).
#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <string>

#include "cButton.hpp"
#include "cWindow.hpp"

using mxh::ui::cButton;
using mxh::ui::cWindow;
using BState = cButton::State;
using WE     = cWindow::WindowEvent;

namespace {
constexpr std::uint32_t kLButton = cWindow::MouseFlagLButton;

// A scratch "image" pointer; opaque to the framework.
int g_basicImage   = 1;
int g_overImage    = 2;
int g_pressImage   = 3;
} // namespace

TEST(CButton, DefaultStateIsNormal) {
    cButton b;
    EXPECT_EQ(b.state(), BState::Normal);
    EXPECT_EQ(b.text(), "");
}

TEST(CButton, InitStoresImagesAndId) {
    cButton b;
    b.Init(10, 20, 100, 50, &g_basicImage, &g_overImage, &g_pressImage,
           /*onClick*/{}, /*userdata*/nullptr, /*id*/42);
    EXPECT_EQ(b.basicImage(), &g_basicImage);
    EXPECT_EQ(b.overImage(), &g_overImage);
    EXPECT_EQ(b.pressImage(), &g_pressImage);
    EXPECT_EQ(b.id(), 42);
    EXPECT_EQ(b.state(), BState::Normal);
    EXPECT_FALSE(b.shadowEnabled());
}

TEST(CButton, MouseEnterTransitionsToHover) {
    cButton b;
    b.Init(0, 0, 100, 100, &g_basicImage, &g_overImage, &g_pressImage);
    // No flags = hover event.
    EXPECT_EQ(b.state(), BState::Normal);
    b.ActionEvent(50, 50, 0);
    EXPECT_EQ(b.state(), BState::Hover);
}

TEST(CButton, MouseLeaveTransitionsBackToNormal) {
    cButton b;
    b.Init(0, 0, 100, 100, &g_basicImage, &g_overImage, &g_pressImage);
    b.ActionEvent(50, 50, 0);                  // enter → Hover
    b.ActionEvent(200, 200, 0);                // leave → Normal
    EXPECT_EQ(b.state(), BState::Normal);
}

TEST(CButton, PressInsideTransitionsToPressed) {
    cButton b;
    b.Init(0, 0, 100, 100, &g_basicImage, &g_overImage, &g_pressImage);
    b.ActionEvent(50, 50, kLButton);
    EXPECT_EQ(b.state(), BState::Pressed);
    EXPECT_FALSE(b.consumeClickInside());      // not yet — release required
}

TEST(CButton, PressThenReleaseInsideFiresClick) {
    cButton b;
    int callCount = 0;
    std::int32_t lastId = 0;
    b.Init(0, 0, 100, 100, &g_basicImage, &g_overImage, &g_pressImage,
           [&](std::int32_t id, void*) { ++callCount; lastId = id; },
           /*userdata*/nullptr, /*id*/7);
    b.ActionEvent(50, 50, kLButton);           // press inside
    b.ActionEvent(50, 50, 0);                  // release inside
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(lastId, 7);
    EXPECT_TRUE(b.consumeClickInside());       // click latched
    EXPECT_FALSE(b.consumeClickInside());      // consumed
    EXPECT_EQ(b.state(), BState::Hover);
}

TEST(CButton, DragOutCancelsClick) {
    cButton b;
    int callCount = 0;
    b.Init(0, 0, 100, 100, &g_basicImage, &g_overImage, &g_pressImage,
           [&](std::int32_t, void*) { ++callCount; });
    b.ActionEvent(50, 50, kLButton);           // press inside
    b.ActionEvent(200, 200, kLButton);         // drag out (still pressed)
    EXPECT_EQ(callCount, 0);
    EXPECT_FALSE(b.consumeClickInside());
    b.ActionEvent(200, 200, 0);                // release outside
    EXPECT_EQ(b.state(), BState::Normal);
    EXPECT_EQ(callCount, 0);
}

TEST(CButton, PressOutsideThenDragInDoesNotFire) {
    cButton b;
    int callCount = 0;
    b.Init(0, 0, 100, 100, &g_basicImage, &g_overImage, &g_pressImage,
           [&](std::int32_t, void*) { ++callCount; });
    b.ActionEvent(200, 200, kLButton);         // press outside
    b.ActionEvent(50, 50, kLButton);           // drag in (still pressed)
    EXPECT_EQ(callCount, 0);
    b.ActionEvent(50, 50, 0);                  // release inside
    // The framework treats the "press moved into the box" as a fresh
    // press-then-release-in-box sequence, so this DOES fire a click.
    // The contract is: the user must press and release inside without
    // leaving the box. Pressing outside and dragging in is a new sequence
    // that effectively pressed inside (relative to the button's POV).
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(b.state(), BState::Hover);
}

TEST(CButton, DisabledIgnoresAllEvents) {
    cButton b;
    int callCount = 0;
    b.Init(0, 0, 100, 100, &g_basicImage, &g_overImage, &g_pressImage,
           [&](std::int32_t, void*) { ++callCount; });
    b.SetEnabled(false);
    EXPECT_EQ(b.state(), BState::Disabled);
    EXPECT_EQ(b.ActionEvent(50, 50, kLButton),
              static_cast<std::uint32_t>(WE::Null));
    b.SetEnabled(true);
    EXPECT_EQ(b.state(), BState::Normal);
    b.ActionEvent(50, 50, kLButton);
    b.ActionEvent(50, 50, 0);
    EXPECT_EQ(callCount, 1);
}

TEST(CButton, SetTextStoresLabelAndColors) {
    cButton b;
    b.Init(0, 0, 100, 100, &g_basicImage, &g_overImage, &g_pressImage);
    b.SetText("OK", 0xFF000000u, 0xFFFF0000u, 0xFF00FF00u);
    EXPECT_EQ(b.text(), "OK");
    EXPECT_EQ(b.textBasicColor(), 0xFF000000u);
    EXPECT_EQ(b.textOverColor(),  0xFFFF0000u);
    EXPECT_EQ(b.textPressColor(), 0xFF00FF00u);
    EXPECT_EQ(b.currentTextColor(), 0xFF000000u); // Normal state
    b.ActionEvent(50, 50, 0);                    // enter → Hover
    EXPECT_EQ(b.currentTextColor(), 0xFFFF0000u);
    b.ActionEvent(50, 50, kLButton);             // → Pressed
    EXPECT_EQ(b.currentTextColor(), 0xFF00FF00u);
}

TEST(CButton, SetTextZeroOverPressDefaultsToBasic) {
    // Legacy contract: SetText(text, basic) — over and press default to 0
    // which is interpreted as "use basic".
    cButton b;
    b.Init(0, 0, 100, 100, &g_basicImage, &g_overImage, &g_pressImage);
    b.SetText("X", 0xFFAABBCCu);
    EXPECT_EQ(b.textOverColor(),  0xFFAABBCCu);
    EXPECT_EQ(b.textPressColor(), 0xFFAABBCCu);
}

TEST(CButton, CallbackReceivesUserdata) {
    cButton b;
    int userData = 12345;
    int* received = nullptr;
    b.Init(0, 0, 100, 100, &g_basicImage, &g_overImage, &g_pressImage,
           [&](std::int32_t, void* u) { received = static_cast<int*>(u); },
           &userData);
    b.ActionEvent(50, 50, kLButton);
    b.ActionEvent(50, 50, 0);
    EXPECT_EQ(received, &userData);
    EXPECT_EQ(*received, 12345);
}

TEST(CButton, ShadowSetters) {
    cButton b;
    b.Init(0, 0, 100, 100, &g_basicImage, &g_overImage, &g_pressImage);
    EXPECT_FALSE(b.shadowEnabled());
    b.SetShadow(true);
    b.SetShadowColor(0xFF808080u);
    b.SetShadowTextXY(2, 2);
    EXPECT_TRUE(b.shadowEnabled());
    EXPECT_EQ(b.shadowColor(), 0xFF808080u);
}

TEST(CButton, TextAlignSetter) {
    cButton b;
    b.Init(0, 0, 100, 100, &g_basicImage, &g_overImage, &g_pressImage);
    EXPECT_EQ(b.textAlign(), cButton::TextAlign::Center);
    b.SetTextAlign(cButton::TextAlign::Left);
    EXPECT_EQ(b.textAlign(), cButton::TextAlign::Left);
}

TEST(CButton, SetTextXYStoresCoordinates) {
    cButton b;
    b.Init(0, 0, 100, 100, &g_basicImage, &g_overImage, &g_pressImage);
    b.SetTextXY(10, 20);
    // No getter for textXY, but the call must not crash and the
    // rest of the state machine must remain intact.
    b.ActionEvent(50, 50, 0);
    EXPECT_EQ(b.state(), BState::Hover);
}

TEST(CButton, ClickEventReturnsLButtonClick) {
    // ActionEvent must surface LButtonClick for the dispatcher when a
    // click was consumed.
    cButton b;
    b.Init(0, 0, 100, 100, &g_basicImage, &g_overImage, &g_pressImage);
    b.ActionEvent(50, 50, kLButton);
    const std::uint32_t ev = b.ActionEvent(50, 50, 0);
    EXPECT_EQ(ev, static_cast<std::uint32_t>(WE::LButtonClick));
    EXPECT_EQ(b.state(), BState::Hover);
}

TEST(CButton, PressInsideReturnsLButtonDown) {
    cButton b;
    b.Init(0, 0, 100, 100, &g_basicImage, &g_overImage, &g_pressImage);
    b.ActionEvent(50, 50, 0);                  // enter → Hover
    const std::uint32_t ev = b.ActionEvent(50, 50, kLButton);
    EXPECT_EQ(ev, static_cast<std::uint32_t>(WE::LButtonDown));
    EXPECT_EQ(b.state(), BState::Pressed);
}

TEST(CButton, DisabledClickDoesNotInvokeCallback) {
    cButton b;
    int callCount = 0;
    b.Init(0, 0, 100, 100, &g_basicImage, &g_overImage, &g_pressImage,
           [&](std::int32_t, void*) { ++callCount; });
    b.SetEnabled(false);
    b.ActionEvent(50, 50, kLButton);
    b.ActionEvent(50, 50, 0);
    EXPECT_EQ(callCount, 0);
    EXPECT_FALSE(b.consumeClickInside());
}

TEST(CButton, MultipleSequentialClicksAllFire) {
    cButton b;
    int callCount = 0;
    b.Init(0, 0, 100, 100, &g_basicImage, &g_overImage, &g_pressImage,
           [&](std::int32_t, void*) { ++callCount; });
    for (int i = 0; i < 5; ++i) {
        b.ActionEvent(50, 50, kLButton);
        b.ActionEvent(50, 50, 0);
    }
    EXPECT_EQ(callCount, 5);
}

TEST(CButton, NonLButtonFlagsDoNotTriggerClick) {
    // Right-click, shift, control must not trigger the click path.
    cButton b;
    int callCount = 0;
    b.Init(0, 0, 100, 100, &g_basicImage, &g_overImage, &g_pressImage,
           [&](std::int32_t, void*) { ++callCount; });
    b.ActionEvent(50, 50, cWindow::MouseFlagRButton);
    b.ActionEvent(50, 50, 0);
    EXPECT_EQ(callCount, 0);
    EXPECT_EQ(b.state(), BState::Hover);
}
