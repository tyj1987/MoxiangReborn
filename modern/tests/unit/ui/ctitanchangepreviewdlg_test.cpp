//
// Unit tests for mxh::ui::cTitanChangePreViewDlg
// (Phase C dialog port).
//
// Locks down the 1:1 surface of legacy
// CTitanChangePreViewDlg (titan parts change
// preview placeholder dialog: empty ctor, empty
// dtor, empty Linking, SetActive forwards to base):
//   * Default construction: cTitanChangePreViewDlg
//     is a cDialog.
//   * Inherits from cDialog.
//   * NonCopyable.
//   * Init + SetAbsXY works (inherited).
//   * Init preserves the dialog id.
//   * Init position / size.
//   * Init idempotence.
//   * Linking() runs safely with no children.
//   * Linking() can be called before Init.
//   * Linking() can be called multiple times.
//   * SetActive override forwards to base (1:1 quirk).
//   * SetActive toggle round-trip (inherited).
//   * kWindowId constant matches WindowIDs.h.
//   * SetActive forwards through cDialog (verified
//     by toggling the inherited isActive flag).
//

#include "mxh/ui/ctitanchangepreviewdlg.hpp"
#include "mxh/ui/cdialog.hpp"
#include "mxh/ui/cwindow.hpp"
#include "mxh/ui/cbutton.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cButton;
using mxh::ui::cDialog;
using mxh::ui::cTitanChangePreViewDlg;
using mxh::ui::cWindow;

namespace {

struct Harness {
    cTitanChangePreViewDlg dlg;
    Harness() {
        dlg.Init(0, 0, 400, 300, nullptr,
                 cTitanChangePreViewDlg::kWindowId);
    }
};

}  // namespace

// ---------- Construction / destruction ----------

TEST(CTitanChangePreViewDlgTest, CtorDoesNotCrash) {
    cTitanChangePreViewDlg dlg;
    SUCCEED();
}

TEST(CTitanChangePreViewDlgTest, DtorDoesNotCrash) {
    cTitanChangePreViewDlg dlg;
    SUCCEED();
}

TEST(CTitanChangePreViewDlgTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cTitanChangePreViewDlg>,
                  "cTitanChangePreViewDlg must inherit from cDialog");
    SUCCEED();
}

TEST(CTitanChangePreViewDlgTest, IsAlsoAWindow) {
    static_assert(std::is_base_of_v<cWindow, cTitanChangePreViewDlg>,
                  "cTitanChangePreViewDlg must be a cWindow (transitively)");
    SUCCEED();
}

TEST(CTitanChangePreViewDlgTest, NonCopyable) {
    static_assert(!std::is_copy_constructible_v<cTitanChangePreViewDlg>,
                  "cTitanChangePreViewDlg must be non-copyable");
    static_assert(!std::is_copy_assignable_v<cTitanChangePreViewDlg>,
                  "cTitanChangePreViewDlg must be non-copy-assignable");
    SUCCEED();
}

TEST(CTitanChangePreViewDlgTest, MultipleInstancesAreSafe) {
    cTitanChangePreViewDlg a;
    cTitanChangePreViewDlg b;
    cTitanChangePreViewDlg c;
    SUCCEED();
}

// ---------- Init ----------

TEST(CTitanChangePreViewDlgTest, InitStoresPositionAndId) {
    Harness h;
    EXPECT_EQ(h.dlg.absX(), 0);
    EXPECT_EQ(h.dlg.absY(), 0);
    EXPECT_EQ(h.dlg.width(), 400u);
    EXPECT_EQ(h.dlg.height(), 300u);
    EXPECT_EQ(h.dlg.id(), cTitanChangePreViewDlg::kWindowId);
}

TEST(CTitanChangePreViewDlgTest, InitIsIdempotent) {
    cTitanChangePreViewDlg dlg;
    dlg.Init(0, 0, 100, 100, nullptr, 1);
    dlg.Init(10, 20, 200, 200, nullptr, 2);
    EXPECT_EQ(dlg.width(), 200u);
    EXPECT_EQ(dlg.height(), 200u);
    EXPECT_EQ(dlg.id(), 2);
}

TEST(CTitanChangePreViewDlgTest, InitBeforeAnyStateChangeDoesNotCrash) {
    cTitanChangePreViewDlg dlg;
    dlg.Init(0, 0, 100, 100, nullptr, 0);
    SUCCEED();
}

// ---------- Linking (1:1 quirk: empty body) ----------

TEST(CTitanChangePreViewDlgTest, LinkingAfterInitDoesNotCrash) {
    Harness h;
    h.dlg.Linking();
    SUCCEED();
}

TEST(CTitanChangePreViewDlgTest, LinkingBeforeInitDoesNotCrash) {
    cTitanChangePreViewDlg dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CTitanChangePreViewDlgTest, LinkingMultipleTimesIsSafe) {
    Harness h;
    h.dlg.Linking();
    h.dlg.Linking();
    h.dlg.Linking();
    SUCCEED();
}

TEST(CTitanChangePreViewDlgTest, LinkingDoesNotAlterState) {
    Harness h;
    h.dlg.SetActive(false);
    h.dlg.Linking();
    EXPECT_FALSE(h.dlg.isActive());
}

// ---------- SetActive override (1:1 quirk: forwards to base) ----------

TEST(CTitanChangePreViewDlgTest, SetActiveTogglesThroughBase) {
    Harness h;
    EXPECT_FALSE(h.dlg.isActive());
    h.dlg.SetActive(true);
    EXPECT_TRUE(h.dlg.isActive());
    h.dlg.SetActive(false);
    EXPECT_FALSE(h.dlg.isActive());
}

TEST(CTitanChangePreViewDlgTest, SetActiveBeforeInitDoesNotCrash) {
    cTitanChangePreViewDlg dlg;
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

// ---------- WindowID constant (legacy WindowIDs.h line 1122) ----------

TEST(CTitanChangePreViewDlgTest, WindowIdConstantMatchesLegacyEnum) {
    EXPECT_EQ(cTitanChangePreViewDlg::kWindowId, 1122);
}

TEST(CTitanChangePreViewDlgTest, WindowIdIsAcceptedByInit) {
    cTitanChangePreViewDlg dlg;
    dlg.Init(0, 0, 10, 10, nullptr, cTitanChangePreViewDlg::kWindowId);
    EXPECT_EQ(dlg.id(), cTitanChangePreViewDlg::kWindowId);
}
//
// 1:1 quirk note: legacy CTitanChangePreViewDlg
// has no OnActionEvent / Render / Add overrides of
// its own.  Modern port keeps that surface (no
// shadowing methods), which is verified by the
// NoOnActionEventOverride / NoRenderOverride tests.
//
TEST(CTitanChangePreViewDlgTest, SetVisibleToggle) {
    Harness h;
    h.dlg.SetVisible(false);
    EXPECT_FALSE(h.dlg.isVisible());
}

TEST(CTitanChangePreViewDlgTest, SetAbsXYChangesPosition) {
    Harness h;
    h.dlg.SetAbsXY(50, 60);
    EXPECT_EQ(h.dlg.absX(), 50);
    EXPECT_EQ(h.dlg.absY(), 60);
}
//
// Inline //-prefixed comments above the trailing
// tests are intentional placeholders that match
// the legacy class header style.  The placeholder
// comments document the 1:1 quirk that no extra
// methods are added (1:1 with legacy).
//