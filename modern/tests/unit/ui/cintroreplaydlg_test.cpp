//
// Unit tests for mxh::ui::cIntroReplayDlg (Phase C dialog port).
//
// Locks down the 1:1 surface of legacy CIntroReplayDlg
// (intro replay placeholder dialog: empty ctor, empty
// dtor, empty Linking):
//   * Default construction: cIntroReplayDlg is a cDialog.
//   * Inherits from cDialog.
//   * NonCopyable.
//   * Linking is a no-op (does not crash, no state change).
//   * Linking can be called before Init.
//   * Linking can be called after Init.
//   * Linking can be called multiple times.
//   * Init + SetAbsXY works (inherited).
//   * SetActive / isActive (inherited).
//   * SetVisible / isVisible (inherited).
//   * SetEnabled / SetDisable / isEnabled (inherited).
//   * SetAlpha / alpha (inherited).
//   * SetOptionAlpha / optionAlpha (inherited).
//   * Caption rect (inherited).
//   * Add children (inherited tree management).
//   * findWindowById (inherited).
//   * requestClose / IsAutoClose (inherited).
//   * Init preserves dialog id.
//   * Init idempotence.
//   * Init position / size.
//   * Linking does not affect the auto-close flag.
//   * Linking does not change dialog id.
//   * Linking does not affect the disabled state.
//   * Linking does not change active state.
//

#include "mxh/ui/cintroreplaydlg.hpp"
#include "mxh/ui/cdialog.hpp"
#include "mxh/ui/cwindow.hpp"
#include "mxh/ui/cbutton.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cButton;
using mxh::ui::cDialog;
using mxh::ui::cIntroReplayDlg;
using mxh::ui::cWindow;

namespace {

struct Harness {
    cIntroReplayDlg dlg;
    Harness() {
        dlg.Init(0, 0, 400, 300, nullptr, 7777);
    }
};

}  // namespace

// ---------- Construction / destruction ----------

TEST(CIntroReplayDlgTest, CtorDoesNotCrash) {
    cIntroReplayDlg dlg;
    SUCCEED();
}

TEST(CIntroReplayDlgTest, DtorDoesNotCrash) {
    cIntroReplayDlg dlg;
    SUCCEED();
}

TEST(CIntroReplayDlgTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cIntroReplayDlg>,
                  "cIntroReplayDlg must inherit from cDialog");
    SUCCEED();
}

TEST(CIntroReplayDlgTest, IsAlsoAWindow) {
    static_assert(std::is_base_of_v<cWindow, cIntroReplayDlg>,
                  "cIntroReplayDlg must be a cWindow (transitively)");
    SUCCEED();
}

TEST(CIntroReplayDlgTest, NonCopyable) {
    static_assert(!std::is_copy_constructible_v<cIntroReplayDlg>,
                  "cIntroReplayDlg must be non-copyable");
    static_assert(!std::is_copy_assignable_v<cIntroReplayDlg>,
                  "cIntroReplayDlg must be non-copy-assignable");
    SUCCEED();
}

// ---------- Init ----------

TEST(CIntroReplayDlgTest, InitStoresPosition) {
    Harness h;
    h.dlg.Init(0, 0, 400, 300, nullptr, 7777);
    EXPECT_EQ(h.dlg.absX(), 0);
    EXPECT_EQ(h.dlg.absY(), 0);
    EXPECT_EQ(h.dlg.width(), 400u);
    EXPECT_EQ(h.dlg.height(), 300u);
    EXPECT_EQ(h.dlg.id(), 7777);
}

TEST(CIntroReplayDlgTest, InitIsIdempotent) {
    cIntroReplayDlg dlg;
    dlg.Init(0, 0, 100, 100, nullptr, 1);
    dlg.Init(10, 20, 200, 200, nullptr, 2);
    EXPECT_EQ(dlg.width(), 200u);
    EXPECT_EQ(dlg.height(), 200u);
    EXPECT_EQ(dlg.id(), 2);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

// ---------- Linking ----------

TEST(CIntroReplayDlgTest, LinkingIsNoOp) {
    Harness h;
    // 1:1 quirk: legacy Linking body is empty.
    h.dlg.Linking();
    SUCCEED();
}

TEST(CIntroReplayDlgTest, LinkingBeforeInitDoesNotCrash) {
    cIntroReplayDlg dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CIntroReplayDlgTest, LinkingIsIdempotent) {
    Harness h;
    h.dlg.Linking();
    h.dlg.Linking();
    h.dlg.Linking();
    SUCCEED();
}

TEST(CIntroReplayDlgTest, LinkingDoesNotChangeId) {
    Harness h;
    int idBefore = h.dlg.id();
    h.dlg.Linking();
    EXPECT_EQ(h.dlg.id(), idBefore);
}

TEST(CIntroReplayDlgTest, LinkingDoesNotChangeActive) {
    Harness h;
    h.dlg.SetActive(true);
    bool before = h.dlg.isActive();
    h.dlg.Linking();
    EXPECT_EQ(h.dlg.isActive(), before);
}

TEST(CIntroReplayDlgTest, LinkingDoesNotChangeAutoClose) {
    Harness h;
    h.dlg.SetAutoClose(true);
    h.dlg.Linking();
    EXPECT_TRUE(h.dlg.IsAutoClose());
}

TEST(CIntroReplayDlgTest, LinkingDoesNotChangeDisabledState) {
    Harness h;
    h.dlg.SetEnabled(false);
    h.dlg.Linking();
    EXPECT_FALSE(h.dlg.isEnabled());
}

// ---------- SetActive / SetVisible / SetEnabled / SetDisable ----------

TEST(CIntroReplayDlgTest, SetActiveToggle) {
    Harness h;
    EXPECT_FALSE(h.dlg.isActive());
    h.dlg.SetActive(true);
    EXPECT_TRUE(h.dlg.isActive());
    h.dlg.SetActive(false);
    EXPECT_FALSE(h.dlg.isActive());
}

TEST(CIntroReplayDlgTest, SetVisibleToggle) {
    Harness h;
    h.dlg.SetVisible(false);
    EXPECT_FALSE(h.dlg.isVisible());
}

TEST(CIntroReplayDlgTest, SetEnabledToggle) {
    Harness h;
    h.dlg.SetEnabled(false);
    EXPECT_FALSE(h.dlg.isEnabled());
}

TEST(CIntroReplayDlgTest, SetDisableIsAliasForSetEnabled) {
    Harness h;
    h.dlg.SetDisable(true);
    EXPECT_FALSE(h.dlg.isEnabled());
}

// ---------- SetAbsXY / SetRelXY / SetWH ----------

TEST(CIntroReplayDlgTest, SetAbsXYChangesPosition) {
    Harness h;
    h.dlg.SetAbsXY(50, 100);
    EXPECT_EQ(h.dlg.absX(), 50);
    EXPECT_EQ(h.dlg.absY(), 100);
}

// ---------- Alpha ----------

TEST(CIntroReplayDlgTest, SetAlphaStoresValue) {
    Harness h;
    h.dlg.SetAlpha(64);
    EXPECT_EQ(h.dlg.alpha(), 64);
}

TEST(CIntroReplayDlgTest, SetOptionAlphaStoresValue) {
    Harness h;
    h.dlg.SetOptionAlpha(0xAABBCCDDu);
    EXPECT_EQ(h.dlg.optionAlpha(), 0xAABBCCDDu);
}

// ---------- Children tree management ----------

TEST(CIntroReplayDlgTest, AddChildStoresInTree) {
    Harness h;
    auto btn = std::make_unique<cButton>();
    btn->Init(10, 10, 50, 30, nullptr, nullptr, nullptr, nullptr, nullptr, 88);
    h.dlg.Add(std::move(btn));
    EXPECT_EQ(h.dlg.childCount(), 1u);
    cWindow* found = h.dlg.findWindowById(88);
    EXPECT_NE(found, nullptr);
}

TEST(CIntroReplayDlgTest, FindWindowByIdReturnsSelfWhenMatching) {
    Harness h;
    cWindow* found = h.dlg.findWindowById(7777);
    EXPECT_EQ(found, static_cast<cWindow*>(&h.dlg));
}

// ---------- Auto-close / requestClose ----------

TEST(CIntroReplayDlgTest, RequestCloseLatchesTrue) {
    Harness h;
    h.dlg.requestClose();
    EXPECT_TRUE(h.dlg.closeRequested());
    h.dlg.clearCloseRequest();
    EXPECT_FALSE(h.dlg.closeRequested());
}

TEST(CIntroReplayDlgTest, IsAutoCloseStoresValue) {
    Harness h;
    h.dlg.SetAutoClose(true);
    EXPECT_TRUE(h.dlg.IsAutoClose());
    h.dlg.SetAutoClose(false);
    EXPECT_FALSE(h.dlg.IsAutoClose());
}