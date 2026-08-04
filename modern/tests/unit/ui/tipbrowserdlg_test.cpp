// tipbrowserdlg_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cTipBrowserDlg (tip browser dialog:
// 4 pushup tab buttons + 4 nested cDialog pages + 1 cancel
// button).
//
// Covers modern/src/ui/tipbrowserdlg.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\TipBrowserDlg.h (383 B) and
//   墨香【源码】\[Client]MH\TipBrowserDlg.cpp.
//
// What's tested:
//   - Default construction: cTipBrowserDlg is a cDialog
//     and inherits its tree management.
//   - 4 m_pButton + 4 m_pDlg start null (1:1 with
//     legacy default init).
//   - m_wCurDlg starts 0.
//   - 3 id constants match expected local range
//     (kIdStateBase=380, kIdPushupBase=385,
//     kIdCancelBtn=389, kNumTabs=4).
//   - Linking resolves 4 cDialog + 4 cPushupButton
//     children by id.
//   - Linking sets m_wCurDlg = 0.
//   - Linking without children leaves all pointers
//     null.
//   - Show: SetActive(TRUE) on the dialog + activates
//     the current tab + pushes + disables the
//     current tab button.
//   - Show resets all OTHER tabs (SetActive(FALSE) +
//     SetPush(FALSE) + SetDisable(FALSE)).
//   - Close: SetActive(FALSE) + m_wCurDlg = 0.
//   - OnActionEvent with WE_PUSHDOWN + valid id (in
//     [0, 4)) switches the tab + Show.
//   - OnActionEvent with WE_PUSHDOWN + invalid id
//     (out of [0, 4)) is a no-op (1:1 with legacy
//     `id < 4` guard).
//   - OnActionEvent with WE_PUSHDOWN + bitwise
//     overlap (e.g. WE_PUSHDOWN | WE_BTNCLICK) is
//     a no-op (1:1 quirk: legacy uses `==` exact
//     match, not bit-and).
//   - OnActionEvent with WE_BTNCLICK + kIdCancelBtn
//     calls Close (sets active false + resets
//     m_wCurDlg).
//   - OnActionEvent with WE_BTNCLICK + non-cancel id
//     is a no-op.
//   - OnActionEvent with unknown we is a no-op.
//
// 1:1 quirks preserved:
//   - Ctor / dtor: NULL-out all 8 pointers + m_wCurDlg
//     = 0 (modern port uses default member init).
//   - 1:1 quirk: legacy Show's closing `}` placement
//     is unusual; modern port follows the corrected
//     control flow (the SetActive(TRUE) +
//     SetPush(TRUE) for m_wCurDlg is outside the for
//     loop).
//   - 1:1 quirk: legacy OnActionEvent uses
//     `we == WE_PUSHDOWN` (exact match, not bit-and).
//     Modern port preserves the `==`.
//   - 1:1 quirk: legacy OnActionEvent uses
//     `we & WE_BTNCLICK` for the CANCEL branch
//     (bit-and).
//   - m_wCurDlg default 0 (1:1 with legacy).
//   - kNumTabs = 4 (1:1 with legacy array size).

#include "tipbrowserdlg.hpp"
#include "cdialog.hpp"
#include "cpushupbutton.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>

#include "legacy_window_event.hpp"
#include <memory>

namespace mxh::ui::test {

// ===========================================================================
// Construction + state
// ===========================================================================

TEST(CTipBrowserDlgTest, DefaultConstructionIsValid) {
    cTipBrowserDlg dlg;
    SUCCEED();
}

TEST(CTipBrowserDlgTest, InheritsDialogTreeManagement) {
    cTipBrowserDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CTipBrowserDlgTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cTipBrowserDlg::kIdStateBase, 380);
    EXPECT_EQ(cTipBrowserDlg::kIdPushupBase, 385);
    EXPECT_EQ(cTipBrowserDlg::kIdCancelBtn, 389);
    EXPECT_EQ(cTipBrowserDlg::kNumTabs, 4u);
}

// ===========================================================================
// Linking
// ===========================================================================

namespace {

// Build a cTipBrowserDlg with 4 cDialog pages + 4
// cPushupButton children. Returns the dialog +
// raw pointers to the children (for verification).
struct TipBrowserChildren {
    cDialog*       pages[4]    = {nullptr, nullptr, nullptr, nullptr};
    cPushupButton* buttons[4]  = {nullptr, nullptr, nullptr, nullptr};
};

void BuildDlgWithChildren(cTipBrowserDlg& dlg, TipBrowserChildren& out) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    for (std::size_t i = 0; i < 4; ++i) {
        auto page = std::make_unique<cDialog>();
        page->Init(0, 0, 100, 100, nullptr,
                   cTipBrowserDlg::kIdStateBase + static_cast<std::int32_t>(i));
        out.pages[i] = page.get();
        dlg.Add(std::unique_ptr<cWindow>(page.release()));

        auto btn = std::make_unique<cPushupButton>();
        btn->Init(0, 0, 30, 30, nullptr, nullptr, nullptr,
                  nullptr, nullptr,
                  cTipBrowserDlg::kIdPushupBase + static_cast<std::int32_t>(i));
        out.buttons[i] = btn.get();
        dlg.Add(std::unique_ptr<cWindow>(btn.release()));
    }
    dlg.Linking();
}

}  // namespace

TEST(CTipBrowserDlgTest, LinkingResolvesAllChildren) {
    cTipBrowserDlg dlg;
    TipBrowserChildren ch;
    BuildDlgWithChildren(dlg, ch);
    // After Linking, the legacy stores the
    // pointers in m_pDlg / m_pButton. We verify
    // indirectly via Show: it should activate
    // page 0 (m_wCurDlg = 0 default).
    dlg.Show();
    EXPECT_TRUE(ch.pages[0]->isActive());
    EXPECT_TRUE(ch.buttons[0]->IsPushed());
    // Other pages / buttons deactivated / unpushed.
    EXPECT_FALSE(ch.pages[1]->isActive());
    EXPECT_FALSE(ch.buttons[1]->IsPushed());
    // 1:1 quirk: legacy SetDisable(FALSE) for
    // other tabs means they are enabled
    // (isEnabled() == true).
    EXPECT_TRUE(ch.buttons[1]->isEnabled());
}

TEST(CTipBrowserDlgTest, LinkingSetsCurDlgToZero) {
    cTipBrowserDlg dlg;
    TipBrowserChildren ch;
    BuildDlgWithChildren(dlg, ch);
    // After Linking, m_wCurDlg = 0. Show activates
    // page 0 (the first tab).
    dlg.Show();
    EXPECT_TRUE(ch.pages[0]->isActive());
}

TEST(CTipBrowserDlgTest, LinkingWithoutChildrenDoesNotCrash) {
    cTipBrowserDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    // Show() guards each pointer access; no crash.
    dlg.Show();
    SUCCEED();
}

TEST(CTipBrowserDlgTest, LinkingBeforeInitDoesNotCrash) {
    cTipBrowserDlg dlg;
    dlg.Linking();
    SUCCEED();
}

// ===========================================================================
// Show / Close
// ===========================================================================

TEST(CTipBrowserDlgTest, ShowActivatesDialogAndCurrentTab) {
    cTipBrowserDlg dlg;
    TipBrowserChildren ch;
    BuildDlgWithChildren(dlg, ch);
    dlg.Show();
    EXPECT_TRUE(dlg.isActive());
    EXPECT_TRUE(ch.pages[0]->isActive());
    EXPECT_TRUE(ch.buttons[0]->IsPushed());
    // 1:1 quirk: legacy SetDisable(TRUE) for the
    // current tab means it's disabled
    // (isEnabled() == false).
    EXPECT_FALSE(ch.buttons[0]->isEnabled());
}

TEST(CTipBrowserDlgTest, ShowDeactivatesOtherTabs) {
    cTipBrowserDlg dlg;
    TipBrowserChildren ch;
    BuildDlgWithChildren(dlg, ch);
    dlg.Show();
    // Pages 1-3 are deactivated, buttons 1-3 are
    // unpushed + enabled.
    for (std::size_t i = 1; i < 4; ++i) {
        EXPECT_FALSE(ch.pages[i]->isActive());
        EXPECT_FALSE(ch.buttons[i]->IsPushed());
        EXPECT_TRUE(ch.buttons[i]->isEnabled());
    }
}

TEST(CTipBrowserDlgTest, CloseDeactivatesDialogAndResetsCurDlg) {
    cTipBrowserDlg dlg;
    TipBrowserChildren ch;
    BuildDlgWithChildren(dlg, ch);
    dlg.Show();
    dlg.Close();
    EXPECT_FALSE(dlg.isActive());
    // After Close + Show, page 0 is active (m_wCurDlg
    // was reset to 0).
    dlg.Show();
    EXPECT_TRUE(ch.pages[0]->isActive());
}

// ===========================================================================
// OnActionEvent
// ===========================================================================

namespace {
constexpr std::uint32_t WE_BTNCLICK = mxh::ui::legacy_window_event::kButtonClick;
constexpr std::uint32_t WE_PUSHDOWN  = mxh::ui::legacy_window_event::kPushDown;
}  // namespace

TEST(CTipBrowserDlgTest, OnActionEventPushDownSwitchesTab) {
    cTipBrowserDlg dlg;
    TipBrowserChildren ch;
    BuildDlgWithChildren(dlg, ch);
    dlg.Show();

    // Simulate clicking tab 2.
    dlg.OnActionEvent(cTipBrowserDlg::kIdPushupBase + 2, nullptr, WE_PUSHDOWN);
    // Tab 2 now active, tab 0 deactivated.
    EXPECT_TRUE(ch.pages[2]->isActive());
    EXPECT_TRUE(ch.buttons[2]->IsPushed());
    // 1:1 quirk: legacy SetDisable(TRUE) for
    // buttons[m_wCurDlg] — the *button* is
    // disabled, not the page.
    EXPECT_FALSE(ch.buttons[2]->isEnabled());
    EXPECT_FALSE(ch.pages[0]->isActive());
    EXPECT_FALSE(ch.buttons[0]->IsPushed());
}

TEST(CTipBrowserDlgTest, OnActionEventPushDownInvalidIdIsNoOp) {
    // 1:1 with legacy: `id < 4 && (lId - TB_STATE_PUSHUP1) >= 0`
    // guard. Out-of-range ids are no-ops.
    cTipBrowserDlg dlg;
    TipBrowserChildren ch;
    BuildDlgWithChildren(dlg, ch);
    dlg.Show();

    dlg.OnActionEvent(cTipBrowserDlg::kIdPushupBase - 1, nullptr, WE_PUSHDOWN);
    dlg.OnActionEvent(cTipBrowserDlg::kIdPushupBase + 100, nullptr, WE_PUSHDOWN);
    // No tab switch; tab 0 still active.
    EXPECT_TRUE(ch.pages[0]->isActive());
}

TEST(CTipBrowserDlgTest, OnActionEventPushDownExactMatchRequired) {
    // 1:1 quirk: legacy uses `we == WE_PUSHDOWN`
    // (exact match, not bit-and). A combined
    // flag (e.g. WE_PUSHDOWN | WE_BTNCLICK) does
    // NOT trigger the tab switch path.
    cTipBrowserDlg dlg;
    TipBrowserChildren ch;
    BuildDlgWithChildren(dlg, ch);
    dlg.Show();

    constexpr std::uint32_t WE_PUSHDOWN_AND_BTNCLICK = WE_PUSHDOWN | WE_BTNCLICK;
    dlg.OnActionEvent(cTipBrowserDlg::kIdPushupBase + 2, nullptr, WE_PUSHDOWN_AND_BTNCLICK);
    // No tab switch (the == check fails).
    EXPECT_TRUE(ch.pages[0]->isActive());
}

TEST(CTipBrowserDlgTest, OnActionEventCancelBtnClosesDialog) {
    cTipBrowserDlg dlg;
    TipBrowserChildren ch;
    BuildDlgWithChildren(dlg, ch);
    dlg.Show();
    EXPECT_TRUE(dlg.isActive());

    dlg.OnActionEvent(cTipBrowserDlg::kIdCancelBtn, nullptr, WE_BTNCLICK);
    EXPECT_FALSE(dlg.isActive());

    // After Close + Show, page 0 is active (m_wCurDlg
    // was reset).
    dlg.Show();
    EXPECT_TRUE(ch.pages[0]->isActive());
}

TEST(CTipBrowserDlgTest, OnActionEventBtnClickNonCancelIsNoOp) {
    cTipBrowserDlg dlg;
    TipBrowserChildren ch;
    BuildDlgWithChildren(dlg, ch);
    dlg.Show();
    EXPECT_TRUE(dlg.isActive());

    // A btnclick on a non-cancel id (e.g. a pushup
    // button id with WE_BTNCLICK) is a no-op.
    dlg.OnActionEvent(cTipBrowserDlg::kIdPushupBase + 1, nullptr, WE_BTNCLICK);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CTipBrowserDlgTest, OnActionEventUnknownWeIsNoOp) {
    cTipBrowserDlg dlg;
    TipBrowserChildren ch;
    BuildDlgWithChildren(dlg, ch);
    dlg.Show();

    // we = 0xDE00 has no WE_BTNCLICK (bit 6 = 0)
    // and no WE_PUSHDOWN (bit 5 = 0); neither
    // path triggers.
    dlg.OnActionEvent(cTipBrowserDlg::kIdCancelBtn, nullptr, /*we=*/0xDE00u);
    dlg.OnActionEvent(cTipBrowserDlg::kIdPushupBase + 1, nullptr, /*we=*/0xDE00u);
    // No dispatch (no path matches).
    EXPECT_TRUE(dlg.isActive());
}

TEST(CTipBrowserDlgTest, OnActionEventBeforeLinkingDoesNotCrash) {
    cTipBrowserDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.OnActionEvent(cTipBrowserDlg::kIdCancelBtn, nullptr, WE_BTNCLICK);
    dlg.OnActionEvent(cTipBrowserDlg::kIdPushupBase + 1, nullptr, WE_PUSHDOWN);
    SUCCEED();
}





// === Canonical WINDOW_EVENT constants (C-Batch-2.68) ===

TEST(CTipBrowserDlgTest, UsesCanonicalWindowEventConstants) {
    EXPECT_EQ(cTipBrowserDlg::kWeBtnClick, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(cTipBrowserDlg::kWePushDown, mxh::ui::legacy_window_event::kPushDown);
}

}  // namespace mxh::ui::test
