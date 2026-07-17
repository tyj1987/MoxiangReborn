// loadingdlg_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cLoadingDlg (loading screen dialog:
// a 100% empty placeholder).
//
// Covers modern/src/ui/loadingdlg.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\LoadingDlg.h (578 B) and
//   墨香【源码】\[Client]MH\LoadingDlg.cpp.
//
// What's tested:
//   - Default construction: cLoadingDlg is a cDialog
//     and inherits its tree management.
//   - Init + SetAbsXY works.
//   - SetActive / isActive work (inherits cDialog
//     behavior).
//
// 1:1 quirks preserved:
//   - Ctor / dtor body empty (1:1 with legacy empty
//     CLoadingDlg ctor). The class has NO Linking
//     method (1:1 with legacy).
//   - 1:1 quirk: LoadingDlg is the most minimal
//     Tier 2 dialog possible — no fields, no
//     methods beyond ctor + dtor. It exists in
//     the dialog tree to satisfy the "loading"
//     window id during scene transitions.

#include "loadingdlg.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

namespace mxh::ui::test {

TEST(CLoadingDlgTest, DefaultConstructionIsValid) {
    cLoadingDlg dlg;
    // 1:1 quirk: ctor body is empty (legacy also
    // has empty CLoadingDlg() ctor).
    SUCCEED();
}

TEST(CLoadingDlgTest, InheritsDialogTreeManagement) {
    cLoadingDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CLoadingDlgTest, SetActiveToggleRoundTrip) {
    cLoadingDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    EXPECT_FALSE(dlg.isActive());
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

}  // namespace mxh::ui::test
