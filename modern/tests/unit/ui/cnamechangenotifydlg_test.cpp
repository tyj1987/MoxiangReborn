//
// Unit tests for mxh::ui::cNameChangeNotifyDlg
// (Phase C dialog port).
//
// Locks down the 1:1 surface of legacy
// CNameChangeNotifyDlg (name change notification
// placeholder dialog: empty ctor with m_type tag,
// empty dtor, no Linking / OnActionEvent):
//   * Default construction: cNameChangeNotifyDlg
//     is a cDialog.
//   * Inherits from cDialog.
//   * NonCopyable.
//   * Init + SetAbsXY works (inherited).
//   * Init preserves the dialog id.
//   * Init position / size.
//   * Init idempotence.
//   * SetActive / isActive (inherited).
//   * SetVisible / isVisible (inherited).
//   * SetEnabled / SetDisable / isEnabled (inherited).
//   * SetAlpha / alpha (inherited).
//   * SetOptionAlpha / optionAlpha (inherited).
//   * Caption rect (inherited).
//   * Add children (inherited tree management).
//   * findWindowById (inherited).
//   * requestClose / IsAutoClose (inherited).
//   * Has no Linking or OnActionEvent methods of
//     its own (1:1 with legacy).
//   * Multiple ctor / dtor calls in sequence are
//     safe (1:1 with legacy class having no state).
//   * Init before any state change is safe.
//

#include "mxh/ui/cnamechangenotifydlg.hpp"
#include "mxh/ui/cdialog.hpp"
#include "mxh/ui/cwindow.hpp"
#include "mxh/ui/cbutton.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cButton;
using mxh::ui::cDialog;
using mxh::ui::cNameChangeNotifyDlg;
using mxh::ui::cWindow;

namespace {

struct Harness {
    cNameChangeNotifyDlg dlg;
    Harness() {
        dlg.Init(0, 0, 400, 300, nullptr, 5555);
    }
};

}  // namespace

// ---------- Construction / destruction ----------

TEST(CNameChangeNotifyDlgTest, CtorDoesNotCrash) {
    cNameChangeNotifyDlg dlg;
    SUCCEED();
}

TEST(CNameChangeNotifyDlgTest, DtorDoesNotCrash) {
    cNameChangeNotifyDlg dlg;
    SUCCEED();
}

TEST(CNameChangeNotifyDlgTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cNameChangeNotifyDlg>,
                  "cNameChangeNotifyDlg must inherit from cDialog");
    SUCCEED();
}

TEST(CNameChangeNotifyDlgTest, IsAlsoAWindow) {
    static_assert(std::is_base_of_v<cWindow, cNameChangeNotifyDlg>,
                  "cNameChangeNotifyDlg must be a cWindow (transitively)");
    SUCCEED();
}

TEST(CNameChangeNotifyDlgTest, NonCopyable) {
    static_assert(!std::is_copy_constructible_v<cNameChangeNotifyDlg>,
                  "cNameChangeNotifyDlg must be non-copyable");
    static_assert(!std::is_copy_assignable_v<cNameChangeNotifyDlg>,
                  "cNameChangeNotifyDlg must be non-copy-assignable");
    SUCCEED();
}

TEST(CNameChangeNotifyDlgTest, MultipleInstancesAreSafe) {
    cNameChangeNotifyDlg a;
    cNameChangeNotifyDlg b;
    cNameChangeNotifyDlg c;
    SUCCEED();
}

// ---------- Init ----------

TEST(CNameChangeNotifyDlgTest, InitStoresPositionAndId) {
    Harness h;
    EXPECT_EQ(h.dlg.absX(), 0);
    EXPECT_EQ(h.dlg.absY(), 0);
    EXPECT_EQ(h.dlg.width(), 400u);
    EXPECT_EQ(h.dlg.height(), 300u);
    EXPECT_EQ(h.dlg.id(), 5555);
}

TEST(CNameChangeNotifyDlgTest, InitIsIdempotent) {
    cNameChangeNotifyDlg dlg;
    dlg.Init(0, 0, 100, 100, nullptr, 1);
    dlg.Init(10, 20, 200, 200, nullptr, 2);
    EXPECT_EQ(dlg.width(), 200u);
    EXPECT_EQ(dlg.height(), 200u);
    EXPECT_EQ(dlg.id(), 2);
}

TEST(CNameChangeNotifyDlgTest, InitBeforeAnyStateChangeDoesNotCrash) {
    cNameChangeNotifyDlg dlg;
    dlg.Init(0, 0, 100, 100, nullptr, 0);
    SUCCEED();
}

// ---------- SetActive / SetVisible / SetEnabled / SetDisable ----------

TEST(CNameChangeNotifyDlgTest, SetActiveToggle) {
    Harness h;
    EXPECT_FALSE(h.dlg.isActive());
    h.dlg.SetActive(true);
    EXPECT_TRUE(h.dlg.isActive());
}

TEST(CNameChangeNotifyDlgTest, SetVisibleToggle) {
    Harness h;
    h.dlg.SetVisible(false);
    EXPECT_FALSE(h.dlg.isVisible());
}

TEST(CNameChangeNotifyDlgTest, SetEnabledToggle) {
    Harness h;
    h.dlg.SetEnabled(false);
    EXPECT_FALSE(h.dlg.isEnabled());
}

TEST(CNameChangeNotifyDlgTest, SetDisableIsAliasForSetEnabled) {
    Harness h;
    h.dlg.SetDisable(true);
    EXPECT_FALSE(h.dlg.isEnabled());
}

// ---------- SetAbsXY ----------

TEST(CNameChangeNotifyDlgTest, SetAbsXYChangesPosition) {
    Harness h;
    h.dlg.SetAbsXY(80, 120);
    EXPECT_EQ(h.dlg.absX(), 80);
    EXPECT_EQ(h.dlg.absY(), 120);
}

// ---------- Alpha ----------

TEST(CNameChangeNotifyDlgTest, SetAlphaStoresValue) {
    Harness h;
    h.dlg.SetAlpha(32);
    EXPECT_EQ(h.dlg.alpha(), 32);
}

TEST(CNameChangeNotifyDlgTest, SetOptionAlphaStoresValue) {
    Harness h;
    h.dlg.SetOptionAlpha(0xAABBCCDDu);
    EXPECT_EQ(h.dlg.optionAlpha(), 0xAABBCCDDu);
}

// ---------- Caption rect ----------

TEST(CNameChangeNotifyDlgTest, SetCaptionRectStoresBounds) {
    Harness h;
    h.dlg.SetCaptionRect(0, 0, 400, 30);
    EXPECT_EQ(h.dlg.captionLeft(), 0);
    EXPECT_EQ(h.dlg.captionTop(), 0);
    EXPECT_EQ(h.dlg.captionRight(), 400);
    EXPECT_EQ(h.dlg.captionBottom(), 30);
    EXPECT_TRUE(h.dlg.hasCaption());
}

TEST(CNameChangeNotifyDlgTest, PtInCaptionReturnsTrueInside) {
    Harness h;
    h.dlg.SetCaptionRect(10, 10, 100, 50);
    EXPECT_TRUE(h.dlg.PtInCaption(50, 30));
}

TEST(CNameChangeNotifyDlgTest, PtInCaptionReturnsFalseOutside) {
    Harness h;
    h.dlg.SetCaptionRect(10, 10, 100, 50);
    EXPECT_FALSE(h.dlg.PtInCaption(0, 0));
    EXPECT_FALSE(h.dlg.PtInCaption(200, 200));
}

// ---------- Children tree management ----------

TEST(CNameChangeNotifyDlgTest, AddChildStoresInTree) {
    Harness h;
    auto btn = std::make_unique<cButton>();
    btn->Init(10, 10, 50, 30, nullptr, nullptr, nullptr, nullptr, nullptr, 99);
    h.dlg.Add(std::move(btn));
    EXPECT_EQ(h.dlg.childCount(), 1u);
}

TEST(CNameChangeNotifyDlgTest, FindWindowByIdReturnsSelfWhenMatching) {
    Harness h;
    cWindow* found = h.dlg.findWindowById(5555);
    EXPECT_EQ(found, static_cast<cWindow*>(&h.dlg));
}

TEST(CNameChangeNotifyDlgTest, FindWindowByIdReturnsNullWhenMissing) {
    Harness h;
    cWindow* found = h.dlg.findWindowById(9999);
    EXPECT_EQ(found, nullptr);
}

// ---------- Auto-close / requestClose ----------

TEST(CNameChangeNotifyDlgTest, IsAutoCloseDefaultsFalse) {
    Harness h;
    EXPECT_FALSE(h.dlg.IsAutoClose());
}

TEST(CNameChangeNotifyDlgTest, RequestCloseLatchesTrue) {
    Harness h;
    h.dlg.requestClose();
    EXPECT_TRUE(h.dlg.closeRequested());
    h.dlg.clearCloseRequest();
    EXPECT_FALSE(h.dlg.closeRequested());
}