// tests/unit/ui/cwindow_test.cpp
// Phase 6.0 unit tests for the modern mxh::ui::cWindow framework skeleton.
// All tests are CPU-side; no D3D11 / GPU dependency.
#include <gtest/gtest.h>

#include <memory>
#include <utility>

#include "cObject.hpp"
#include "cWindow.hpp"

using mxh::ui::cObject;
using mxh::ui::cWindow;
using WE = cWindow::WindowEvent;

namespace {
// A minimal concrete cWindow subclass for verifying that virtual methods
// route correctly through the framework.
class CountingWindow : public cWindow {
public:
    int renderCount = 0;
    std::uint32_t lastKeyboardEvent = 0xDEADBEEFu;
    void Render() override { ++renderCount; }
    std::uint32_t ActionKeyboardEvent() override {
        lastKeyboardEvent = static_cast<std::uint32_t>(WE::KeyDown);
        return lastKeyboardEvent;
    }
};
} // namespace

TEST(CWindow, DefaultConstruction) {
    cWindow w;
    EXPECT_EQ(w.id(), 0);
    EXPECT_EQ(w.absX(), 0);
    EXPECT_EQ(w.absY(), 0);
    EXPECT_EQ(w.width(), 0u);
    EXPECT_EQ(w.height(), 0u);
    EXPECT_FALSE(w.hasFocus());
    EXPECT_FALSE(w.isMovable());
    EXPECT_FALSE(w.isDepend());
    EXPECT_TRUE(w.isVisible());
    EXPECT_TRUE(w.isEnabled());
    EXPECT_EQ(w.childCount(), 0u);
    EXPECT_EQ(w.basicImage(), nullptr);
}

TEST(CWindow, InitStoresDimensions) {
    cWindow w;
    int dummyImage = 0;
    w.Init(10, 20, 100, 50, &dummyImage, 42);
    EXPECT_EQ(w.absX(), 10);
    EXPECT_EQ(w.absY(), 20);
    EXPECT_EQ(w.width(), 100u);
    EXPECT_EQ(w.height(), 50u);
    EXPECT_EQ(w.id(), 42);
    EXPECT_EQ(w.basicImage(), &dummyImage);
    // Init also resets relX/relY/validX/validY to match absX/absY
    // (legacy engine sets all three on construction).
    EXPECT_EQ(w.relX(), 10);
    EXPECT_EQ(w.relY(), 20);
    EXPECT_EQ(w.validX(), 10);
    EXPECT_EQ(w.validY(), 20);
}

TEST(CWindow, PtInWindowInside) {
    cWindow w;
    w.Init(0, 0, 100, 50);
    EXPECT_TRUE(w.PtInWindow(50, 25));
    EXPECT_TRUE(w.PtInWindow(1, 1));
    EXPECT_TRUE(w.PtInWindow(99, 49));
}

TEST(CWindow, PtInWindowOutside) {
    cWindow w;
    w.Init(0, 0, 100, 50);
    EXPECT_FALSE(w.PtInWindow(200, 200));
    EXPECT_FALSE(w.PtInWindow(101, 25));
    EXPECT_FALSE(w.PtInWindow(50, 51));
    EXPECT_FALSE(w.PtInWindow(-1, 25));
    EXPECT_FALSE(w.PtInWindow(50, -1));
}

TEST(CWindow, PtInWindowBoundary) {
    // Inclusive on the edge — legacy engine contract.
    cWindow w;
    w.Init(10, 20, 100, 50);
    EXPECT_TRUE(w.PtInWindow(10, 20));    // top-left corner
    EXPECT_TRUE(w.PtInWindow(110, 70));   // bottom-right corner
    EXPECT_TRUE(w.PtInWindow(110, 20));   // top-right corner
    EXPECT_TRUE(w.PtInWindow(10, 70));    // bottom-left corner
    // Just outside the boundary must be false.
    EXPECT_FALSE(w.PtInWindow(111, 70));
    EXPECT_FALSE(w.PtInWindow(10, 71));
}

TEST(CWindow, PtInWindowSizeZero) {
    // Degenerate window: only (x, y) == the origin counts.
    cWindow w;
    w.Init(5, 5, 0, 0);
    EXPECT_TRUE(w.PtInWindow(5, 5));
    EXPECT_FALSE(w.PtInWindow(6, 5));
    EXPECT_FALSE(w.PtInWindow(5, 6));
}

TEST(CWindow, AddChildStoresAndParents) {
    cWindow parent;
    parent.Init(0, 0, 200, 200, nullptr, 1);
    auto child = std::make_unique<cWindow>();
    child->Init(10, 10, 50, 50, nullptr, 2);
    cWindow* rawChild = child.get();
    parent.Add(std::move(child));
    EXPECT_EQ(parent.childCount(), 1u);
    EXPECT_EQ(parent.childAt(0), rawChild);
    EXPECT_EQ(rawChild->parent(), &parent);
}

TEST(CWindow, RemoveChildReturnsOwnership) {
    cWindow parent;
    auto child = std::make_unique<cWindow>();
    child->Init(0, 0, 10, 10);
    cWindow* rawChild = child.get();
    parent.Add(std::move(child));
    auto taken = parent.removeChildAt(0);
    ASSERT_NE(taken, nullptr);
    EXPECT_EQ(taken.get(), rawChild);
    EXPECT_EQ(taken->parent(), nullptr);
    EXPECT_EQ(parent.childCount(), 0u);
}

TEST(CWindow, RemoveChildOutOfRangeReturnsEmpty) {
    cWindow parent;
    auto taken = parent.removeChildAt(99);
    EXPECT_EQ(taken, nullptr);
}

TEST(CWindow, SetFocusToggles) {
    cWindow w;
    w.SetFocus(true);
    EXPECT_TRUE(w.hasFocus());
    w.SetFocus(false);
    EXPECT_FALSE(w.hasFocus());
}

TEST(CWindow, SetMovableToggles) {
    cWindow w;
    EXPECT_FALSE(w.isMovable());
    w.SetMovable(true);
    EXPECT_TRUE(w.isMovable());
    w.SetMovable(false);
    EXPECT_FALSE(w.isMovable());
}

TEST(CWindow, SetVisibleHidesFromDispatch) {
    // A hidden window must not consume mouse events even if the cursor is
    // inside its bounding box.
    cWindow w;
    w.Init(0, 0, 100, 100);
    w.SetVisible(false);
    EXPECT_EQ(w.ActionEvent(50, 50, cWindow::MouseFlagLButton),
              static_cast<std::uint32_t>(WE::Null));
    w.SetVisible(true);
    EXPECT_EQ(w.ActionEvent(50, 50, cWindow::MouseFlagLButton),
              static_cast<std::uint32_t>(WE::LButtonClick));
}

TEST(CWindow, SetEnabledHidesFromDispatch) {
    cWindow w;
    w.Init(0, 0, 100, 100);
    w.SetEnabled(false);
    EXPECT_EQ(w.ActionEvent(50, 50, 0),
              static_cast<std::uint32_t>(WE::Null));
}

TEST(CWindow, SetAbsXYClampsToBox) {
    // SetWH clamps to 0..0xFFFF range. Negative or oversized inputs must
    // not produce an unrepresentable width.
    cWindow w;
    w.SetWH(-5, -10);
    EXPECT_EQ(w.width(), 0u);
    EXPECT_EQ(w.height(), 0u);
    w.SetWH(100, 100);
    EXPECT_EQ(w.width(), 100u);
    w.SetAbsXY(-1000, 2000);
    EXPECT_EQ(w.absX(), -1000);
    EXPECT_EQ(w.absY(), 2000);
}

TEST(CWindow, ActionEventDispatchesToChild) {
    // When a top-level window contains a child that overlaps the cursor,
    // the child must receive the event first (top-down dispatch).
    cWindow parent;
    parent.Init(0, 0, 200, 200, nullptr, 1);
    auto child = std::make_unique<cWindow>();
    child->Init(50, 50, 100, 100, nullptr, 2);
    parent.Add(std::move(child));
    // (75, 75) hits the child but not the parent's edge in any
    // meaningful way; the child should consume the LButtonClick.
    EXPECT_EQ(parent.ActionEvent(75, 75, cWindow::MouseFlagLButton),
              static_cast<std::uint32_t>(WE::LButtonClick));
}

TEST(CWindow, ActionEventReturnsLButtonClickInside) {
    cWindow w;
    w.Init(0, 0, 100, 100);
    EXPECT_EQ(w.ActionEvent(10, 10, cWindow::MouseFlagLButton),
              static_cast<std::uint32_t>(WE::LButtonClick));
}

TEST(CWindow, ActionEventReturnsNullOutside) {
    cWindow w;
    w.Init(0, 0, 100, 100);
    EXPECT_EQ(w.ActionEvent(200, 200, cWindow::MouseFlagLButton),
              static_cast<std::uint32_t>(WE::Null));
}

TEST(CWindow, WindowEventEnumValues) {
    // Stable contract: the wire/dispatch layer depends on these values
    // matching the legacy engine's WE_* codes. If they ever change, the
    // legacy-UI adapter layer needs to be updated in lockstep.
    EXPECT_EQ(static_cast<std::uint32_t>(WE::Null),         0u);
    EXPECT_EQ(static_cast<std::uint32_t>(WE::MouseMove),    1u);
    EXPECT_EQ(static_cast<std::uint32_t>(WE::LButtonDown),  2u);
    EXPECT_EQ(static_cast<std::uint32_t>(WE::LButtonUp),    3u);
    EXPECT_EQ(static_cast<std::uint32_t>(WE::LButtonClick), 4u);
    EXPECT_EQ(static_cast<std::uint32_t>(WE::RButtonDown),  5u);
    EXPECT_EQ(static_cast<std::uint32_t>(WE::RButtonUp),    6u);
    EXPECT_EQ(static_cast<std::uint32_t>(WE::RButtonClick), 7u);
    EXPECT_EQ(static_cast<std::uint32_t>(WE::KeyDown),     10u);
    EXPECT_EQ(static_cast<std::uint32_t>(WE::Char_),       11u);
}

// Sanity-check that a derived class with overridden virtual methods is
// routed correctly. This is the contract that the cButton / cEditBox /
// cListCtrl rewrites in Phase 6.1 will rely on.
TEST(CWindow, SubclassOverrides) {
    CountingWindow w;
    w.Init(0, 0, 100, 100);
    if (w.renderCount != 0) FAIL() << "renderCount not initialized";
    w.Render();
    EXPECT_EQ(w.renderCount, 1);
    const std::uint32_t r1 = w.ActionKeyboardEvent();
    EXPECT_EQ(r1, static_cast<std::uint32_t>(WE::KeyDown));
    EXPECT_EQ(w.lastKeyboardEvent, static_cast<std::uint32_t>(WE::KeyDown));
}
