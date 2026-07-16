// tests/unit/ui/cexitdialog_test.cpp
// Phase 6.x unit tests for the modern mxh::ui::cExitDialog widget — 1:1 port
// of legacy CExitDialog.
//
// What we verify:
//   - Default state (no callback, not active)
//   - Init() resets active to false
//   - SetActive() updates both m_bActive (via cDialog) and the test-facing
//     exitActive() flag
//   - onActiveChanged callback fires exactly once per transition
//   - Setting the same value twice does NOT re-fire the callback
//   - Callback can be cleared (passed {}) silently drops notifications
//   - Callback can be rebound after Init
//   - Callback fires with the new value, not the previous
#include <gtest/gtest.h>

#include "cDialog.hpp"
#include "cExitDialog.hpp"

using mxh::ui::cDialog;
using mxh::ui::cExitDialog;

namespace {
int g_basicImg = 7;
} // namespace

TEST(CExitDialog, DefaultConstruction) {
    cExitDialog d;
    EXPECT_FALSE(d.isActive());
    EXPECT_FALSE(d.exitActive());
    EXPECT_FALSE(d.onActiveChanged());
}

TEST(CExitDialog, InitResetsActiveState) {
    cExitDialog d;
    // Manually flip the state first, then Init() should reset it.
    d.SetActive(true);
    ASSERT_TRUE(d.exitActive());
    d.Init(100, 80, 240, 120, &g_basicImg, 99);
    EXPECT_FALSE(d.exitActive());
    EXPECT_FALSE(d.isActive());
    EXPECT_EQ(d.id(), 99);
    EXPECT_EQ(d.absX(), 100);
    EXPECT_EQ(d.absY(), 80);
    EXPECT_EQ(d.width(), 240u);
    EXPECT_EQ(d.height(), 120u);
    EXPECT_EQ(d.basicImage(), &g_basicImg);
}

TEST(CExitDialog, SetActiveUpdatesBaseAndExitFlags) {
    cExitDialog d;
    d.Init(0, 0, 100, 100, &g_basicImg);

    d.SetActive(true);
    EXPECT_TRUE(d.isActive());
    EXPECT_TRUE(d.exitActive());

    d.SetActive(false);
    EXPECT_FALSE(d.isActive());
    EXPECT_FALSE(d.exitActive());
}

TEST(CExitDialog, CallbackFiresOnTransition) {
    cExitDialog d;
    d.Init(0, 0, 100, 100, &g_basicImg);

    int fireCount = 0;
    bool lastValue = false;
    d.SetOnActiveChanged([&](bool active) {
        ++fireCount;
        lastValue = active;
    });

    d.SetActive(true);
    EXPECT_EQ(fireCount, 1);
    EXPECT_TRUE(lastValue);

    d.SetActive(false);
    EXPECT_EQ(fireCount, 2);
    EXPECT_FALSE(lastValue);
}

TEST(CExitDialog, CallbackDoesNotFireOnSameValue) {
    cExitDialog d;
    d.Init(0, 0, 100, 100, &g_basicImg);

    int fireCount = 0;
    d.SetOnActiveChanged([&](bool) { ++fireCount; });

    d.SetActive(true);
    d.SetActive(true);  // no transition
    d.SetActive(true);  // no transition
    EXPECT_EQ(fireCount, 1);

    d.SetActive(false);
    d.SetActive(false); // no transition
    EXPECT_EQ(fireCount, 2);
}

TEST(CExitDialog, CallbackCanBeCleared) {
    cExitDialog d;
    d.Init(0, 0, 100, 100, &g_basicImg);

    int fireCount = 0;
    d.SetOnActiveChanged([&](bool) { ++fireCount; });
    d.SetActive(true);
    EXPECT_EQ(fireCount, 1);

    d.SetOnActiveChanged({});  // clear callback
    EXPECT_FALSE(d.onActiveChanged());

    d.SetActive(false);
    d.SetActive(true);
    d.SetActive(false);
    EXPECT_EQ(fireCount, 1);  // no further fires

    // Active state still tracks correctly even without callback. The
    // last SetActive(false) above flipped the flag, so both isActive
    // and exitActive should be false here.
    EXPECT_FALSE(d.isActive());
    EXPECT_FALSE(d.exitActive());

    // One more transition to true — state tracks but no callback fires.
    d.SetActive(true);
    EXPECT_EQ(fireCount, 1);
    EXPECT_TRUE(d.isActive());
    EXPECT_TRUE(d.exitActive());
}

TEST(CExitDialog, CallbackCanBeReboundAfterInit) {
    cExitDialog d;
    d.Init(0, 0, 100, 100, &g_basicImg);

    int firstCount  = 0;
    int secondCount = 0;
    d.SetOnActiveChanged([&](bool) { ++firstCount; });
    d.SetActive(true);
    EXPECT_EQ(firstCount, 1);
    EXPECT_EQ(secondCount, 0);

    // Rebind to a different callback.
    d.SetOnActiveChanged([&](bool) { ++secondCount; });
    d.SetActive(false);
    EXPECT_EQ(firstCount, 1);  // old callback detached
    EXPECT_EQ(secondCount, 1);

    d.SetActive(true);
    EXPECT_EQ(firstCount, 1);
    EXPECT_EQ(secondCount, 2);
}

TEST(CExitDialog, CallbackReceivesNewValueNotOld) {
    cExitDialog d;
    d.Init(0, 0, 100, 100, &g_basicImg);

    bool observed = false;
    bool observedWasTrue = false;
    d.SetOnActiveChanged([&](bool active) {
        observed = active;
        // Capture the value of m_bExitActive at callback time. The base
        // cDialog::SetActive() and m_bExitActive are both updated BEFORE
        // the callback fires, so exitActive() must already reflect the
        // new value when the callback runs.
        observedWasTrue = d.exitActive();
    });

    d.SetActive(true);
    EXPECT_TRUE(observed);
    EXPECT_TRUE(observedWasTrue);

    d.SetActive(false);
    EXPECT_FALSE(observed);
    EXPECT_FALSE(observedWasTrue);
}

TEST(CExitDialog, InheritsDialogTreeManagement) {
    // cExitDialog is a cDialog — should still be addable to a parent
    // window tree like any other dialog. This verifies the widget can
    // participate in a real dialog subtree (the legacy engine embeds
    // the exit dialog in the main UI tree).
    cDialog parent;
    parent.Init(0, 0, 800, 600, &g_basicImg, 1);

    // Add via the cWindow::Add machinery (takes ownership via unique_ptr).
    // cExitDialog is non-copyable, so we must construct it in-place via
    // a heap allocation and move it into the parent's tree. (We use
    // new + unique_ptr directly because cExitDialog's ctor isn't
    // accessible to std::make_unique through a public ctor that takes
    // the same arguments — it's default-only.)
    auto* heap = new cExitDialog();
    heap->Init(280, 240, 240, 120, &g_basicImg, 2);
    parent.Add(std::unique_ptr<cExitDialog>(heap));
    EXPECT_EQ(parent.componentCount(), 1u);
    EXPECT_NE(parent.componentAt(0), nullptr);
    EXPECT_EQ(parent.componentAt(0)->id(), 2);
}

TEST(CExitDialog, NonCopyable) {
    cExitDialog d;
    // Compile-time check via std::is_copy_constructible / is_copy_assignable.
    EXPECT_FALSE(std::is_copy_constructible<cExitDialog>::value);
    EXPECT_FALSE(std::is_copy_assignable<cExitDialog>::value);
}
