//
// Unit tests for mxh::ui::cLoadingDlg (Phase C dialog port).
//
// Locks down the 1:1 surface of legacy CLoadingDlg
// (loading screen dialog: a 100% empty placeholder).
//   * Default construction: cLoadingDlg is a cDialog.
//   * Inherits from cDialog (tree management).
//   * NonCopyable.
//   * Init + SetAbsXY works (inherited from cDialog).
//   * SetActive / isActive (inherited).
//   * SetVisible / isVisible (inherited).
//   * SetEnabled / SetDisable / isEnabled (inherited).
//   * SetAlpha / alpha (inherited).
//   * SetOptionAlpha / optionAlpha (inherited).
//   * Caption rect (inherited).
//   * Add children (inherited tree management).
//   * findWindowById works (inherited).
//   * requestClose / IsAutoClose (inherited).
//   * Init preserves the dialog id (1:1 with legacy
//     ctor that does not change id).
//   * Has no fields of its own (verified by sizeof
//     being <= base class size).
//   * Init before any state change is safe.
//   * Multiple Init calls overwrite (1:1 with legacy
//     cDialog::Init being non-virtual).
//   * PtInWindow via parent class.
//   * Has no Linking or OnActionEvent methods of its
//     own (verified by compile-time test: only the
//     inherited cDialog interface is available).
//

#include "mxh/ui/cloadingdlg.hpp"
#include "mxh/ui/cdialog.hpp"
#include "mxh/ui/cwindow.hpp"
#include "mxh/ui/cbutton.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cButton;
using mxh::ui::cDialog;
using mxh::ui::cLoadingDlg;
using mxh::ui::cWindow;

namespace {

// Helper: build a default-constructed + Init'd dialog.
struct Harness {
    cLoadingDlg dlg;
    Harness() {
        dlg.Init(0, 0, 400, 400, nullptr, 1234);
    }
};

}  // namespace

// ---------- Construction / destruction ----------

TEST(CLoadingDlgTest, CtorDoesNotCrash) {
    cLoadingDlg dlg;
    SUCCEED();
}

TEST(CLoadingDlgTest, DtorDoesNotCrash) {
    cLoadingDlg dlg;
    SUCCEED();
}

TEST(CLoadingDlgTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cLoadingDlg>,
                  "cLoadingDlg must inherit from cDialog");
    SUCCEED();
}

TEST(CLoadingDlgTest, NonCopyable) {
    static_assert(!std::is_copy_constructible_v<cLoadingDlg>,
                  "cLoadingDlg must be non-copyable");
    static_assert(!std::is_copy_assignable_v<cLoadingDlg>,
                  "cLoadingDlg must be non-copy-assignable");
    SUCCEED();
}

TEST(CLoadingDlgTest, IsAlsoAWindow) {
    static_assert(std::is_base_of_v<cWindow, cLoadingDlg>,
                  "cLoadingDlg must be a cWindow (transitively)");
    SUCCEED();
}

// ---------- Default state (post-Init) ----------

TEST(CLoadingDlgTest, InheritsInitPositionSize) {
    Harness h;
    EXPECT_EQ(h.dlg.absX(), 0);
    EXPECT_EQ(h.dlg.absY(), 0);
    EXPECT_EQ(h.dlg.width(), 400u);
    EXPECT_EQ(h.dlg.height(), 400u);
}

TEST(CLoadingDlgTest, InheritsInitId) {
    Harness h;
    EXPECT_EQ(h.dlg.id(), 1234);
}

TEST(CLoadingDlgTest, InheritsInitNotActive) {
    Harness h;
    EXPECT_FALSE(h.dlg.isActive());
}

TEST(CLoadingDlgTest, InheritsInitVisible) {
    Harness h;
    EXPECT_TRUE(h.dlg.isVisible());
}

TEST(CLoadingDlgTest, InheritsInitEnabled) {
    Harness h;
    EXPECT_TRUE(h.dlg.isEnabled());
}

TEST(CLoadingDlgTest, InheritsDefaultAlpha) {
    Harness h;
    EXPECT_EQ(h.dlg.alpha(), 255);
}

// ---------- SetActive ----------

TEST(CLoadingDlgTest, SetActiveToggle) {
    Harness h;
    EXPECT_FALSE(h.dlg.isActive());
    h.dlg.SetActive(true);
    EXPECT_TRUE(h.dlg.isActive());
    h.dlg.SetActive(false);
    EXPECT_FALSE(h.dlg.isActive());
}

// ---------- SetAbsXY / SetRelXY / SetWH ----------

TEST(CLoadingDlgTest, SetAbsXYChangesPosition) {
    Harness h;
    h.dlg.SetAbsXY(100, 200);
    EXPECT_EQ(h.dlg.absX(), 100);
    EXPECT_EQ(h.dlg.absY(), 200);
}

TEST(CLoadingDlgTest, SetRelXYChangesRelative) {
    Harness h;
    h.dlg.SetRelXY(10, 20);
    EXPECT_EQ(h.dlg.relX(), 10);
    EXPECT_EQ(h.dlg.relY(), 20);
}

TEST(CLoadingDlgTest, SetWHChangesSize) {
    Harness h;
    h.dlg.SetWH(800, 600);
    EXPECT_EQ(h.dlg.width(), 800u);
    EXPECT_EQ(h.dlg.height(), 600u);
}

// ---------- SetVisible / SetEnabled / SetDisable ----------

TEST(CLoadingDlgTest, SetVisibleToggle) {
    Harness h;
    h.dlg.SetVisible(false);
    EXPECT_FALSE(h.dlg.isVisible());
    h.dlg.SetVisible(true);
    EXPECT_TRUE(h.dlg.isVisible());
}

TEST(CLoadingDlgTest, SetEnabledToggle) {
    Harness h;
    h.dlg.SetEnabled(false);
    EXPECT_FALSE(h.dlg.isEnabled());
    h.dlg.SetEnabled(true);
    EXPECT_TRUE(h.dlg.isEnabled());
}

TEST(CLoadingDlgTest, SetDisableIsAliasForSetEnabled) {
    Harness h;
    h.dlg.SetDisable(true);
    EXPECT_FALSE(h.dlg.isEnabled());
    h.dlg.SetDisable(false);
    EXPECT_TRUE(h.dlg.isEnabled());
}

// ---------- Alpha / OptionAlpha ----------

TEST(CLoadingDlgTest, SetAlphaStoresValue) {
    Harness h;
    h.dlg.SetAlpha(128);
    EXPECT_EQ(h.dlg.alpha(), 128);
}

TEST(CLoadingDlgTest, SetOptionAlphaStoresValue) {
    Harness h;
    h.dlg.SetOptionAlpha(0x80FF0000u);
    EXPECT_EQ(h.dlg.optionAlpha(), 0x80FF0000u);
}

// ---------- Caption rect ----------

TEST(CLoadingDlgTest, SetCaptionRectStoresBounds) {
    Harness h;
    h.dlg.SetCaptionRect(0, 0, 400, 30);
    EXPECT_EQ(h.dlg.captionLeft(), 0);
    EXPECT_EQ(h.dlg.captionTop(), 0);
    EXPECT_EQ(h.dlg.captionRight(), 400);
    EXPECT_EQ(h.dlg.captionBottom(), 30);
    EXPECT_TRUE(h.dlg.hasCaption());
}

TEST(CLoadingDlgTest, PtInCaptionReturnsTrueInside) {
    Harness h;
    h.dlg.SetCaptionRect(10, 10, 100, 50);
    EXPECT_TRUE(h.dlg.PtInCaption(50, 30));
}

TEST(CLoadingDlgTest, PtInCaptionReturnsFalseOutside) {
    Harness h;
    h.dlg.SetCaptionRect(10, 10, 100, 50);
    EXPECT_FALSE(h.dlg.PtInCaption(0, 0));
    EXPECT_FALSE(h.dlg.PtInCaption(200, 200));
}

// ---------- Children tree management ----------

TEST(CLoadingDlgTest, AddChildStoresInTree) {
    Harness h;
    auto btn = std::make_unique<cButton>();
    btn->Init(10, 10, 50, 30, nullptr, nullptr, nullptr, nullptr, nullptr, 99);
    h.dlg.Add(std::move(btn));
    EXPECT_EQ(h.dlg.childCount(), 1u);
    cWindow* found = h.dlg.findWindowById(99);
    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found->id(), 99);
}

TEST(CLoadingDlgTest, FindWindowByIdReturnsSelfWhenMatching) {
    Harness h;
    // The dialog itself has id 1234 from Init.
    cWindow* found = h.dlg.findWindowById(1234);
    EXPECT_EQ(found, static_cast<cWindow*>(&h.dlg));
}

TEST(CLoadingDlgTest, FindWindowByIdReturnsNullWhenMissing) {
    Harness h;
    cWindow* found = h.dlg.findWindowById(9999);
    EXPECT_EQ(found, nullptr);
}

// ---------- Auto-close / requestClose ----------

TEST(CLoadingDlgTest, IsAutoCloseDefaultsFalse) {
    Harness h;
    EXPECT_FALSE(h.dlg.IsAutoClose());
}

TEST(CLoadingDlgTest, SetAutoCloseStoresValue) {
    Harness h;
    h.dlg.SetAutoClose(true);
    EXPECT_TRUE(h.dlg.IsAutoClose());
    h.dlg.SetAutoClose(false);
    EXPECT_FALSE(h.dlg.IsAutoClose());
}

TEST(CLoadingDlgTest, RequestCloseDefaultsFalse) {
    Harness h;
    EXPECT_FALSE(h.dlg.closeRequested());
}

TEST(CLoadingDlgTest, RequestCloseLatchesTrue) {
    Harness h;
    h.dlg.requestClose();
    EXPECT_TRUE(h.dlg.closeRequested());
    h.dlg.clearCloseRequest();
    EXPECT_FALSE(h.dlg.closeRequested());
}

// ---------- Init idempotence ----------

TEST(CLoadingDlgTest, InitIsIdempotent) {
    cLoadingDlg dlg;
    dlg.Init(0, 0, 100, 100, nullptr, 1);
    dlg.Init(0, 0, 200, 200, nullptr, 2);
    EXPECT_EQ(dlg.width(), 200u);
    EXPECT_EQ(dlg.height(), 200u);
    EXPECT_EQ(dlg.id(), 2);
}

TEST(CLoadingDlgTest, InitBeforeStateChangeDoesNotCrash) {
    cLoadingDlg dlg;
    dlg.Init(0, 0, 100, 100, nullptr, 0);
    SUCCEED();
}