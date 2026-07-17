// namechangenotifydlg_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cNameChangeNotifyDlg (name change
// notification dialog: an empty placeholder).
//
// Covers modern/src/ui/namechangenotifydlg.{hpp,cpp}, a 1:1 port
// of
//   墨香【源码】\[Client]MH\NameChangeNotifyDlg.h (652 B) and
//   墨香【源码】\[Client]MH\NameChangeNotifyDlg.cpp.
//
// What's tested:
//   - Default construction: cNameChangeNotifyDlg is
//     a cDialog and inherits its tree management.
//   - Init + SetAbsXY works.
//   - SetActive / isActive work (inherits cDialog
//     behavior).
//
// 1:1 quirks preserved:
//   - Ctor body empty (1:1 with legacy m_type =
//     WT_NAMECHANGENOTIFY_DLG drop, since modern
//     cWindow does not have m_type field).
//   - 1:1 quirk: NameChangeNotifyDlg is an empty
//     placeholder (no Linking, no fields, no
//     methods beyond ctor + dtor). It exists in
//     the dialog tree to satisfy the
//     "name change notify" window id.

#include "namechangenotifydlg.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

namespace mxh::ui::test {

TEST(CNameChangeNotifyDlgTest, DefaultConstructionIsValid) {
    cNameChangeNotifyDlg dlg;
    SUCCEED();
}

TEST(CNameChangeNotifyDlgTest, InheritsDialogTreeManagement) {
    cNameChangeNotifyDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CNameChangeNotifyDlgTest, SetActiveToggleRoundTrip) {
    cNameChangeNotifyDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    EXPECT_FALSE(dlg.isActive());
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

}  // namespace mxh::ui::test
