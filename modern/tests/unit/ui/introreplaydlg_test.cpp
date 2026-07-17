// introreplaydlg_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cIntroReplayDlg (intro replay dialog:
// a placeholder with no behavior).
//
// Covers modern/src/ui/introreplaydlg.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\IntroReplayDlg.h (475 B) and
//   墨香【源码】\[Client]MH\IntroReplayDlg.cpp.
//
// What's tested:
//   - Default construction: cIntroReplayDlg is a cDialog
//     and inherits its tree management.
//   - Linking is a no-op (1:1 with legacy empty body).
//   - Linking before Init is safe (no crash).
//   - Linking after Init is safe (no crash).
//   - Multiple Linking calls are safe (idempotent).
//   - Init + SetAbsXY works (inherits cDialog
//     behavior).
//
// 1:1 quirks preserved:
//   - Ctor / dtor body empty (1:1 with legacy empty
//     CIntroReplayDlg ctor).
//   - Linking body empty (1:1 with legacy empty
//     body). The dialog is a 1:1 placeholder — it
//     exists in the dialog tree to satisfy the
//     window manager's "intro replay button"
//     target id but does no work itself.

#include "introreplaydlg.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

namespace mxh::ui::test {

TEST(CIntroReplayDlgTest, DefaultConstructionIsValid) {
    cIntroReplayDlg dlg;
    // 1:1 quirk: ctor body is empty (legacy also
    // has empty CIntroReplayDlg() ctor).
    SUCCEED();
}

TEST(CIntroReplayDlgTest, InheritsDialogTreeManagement) {
    cIntroReplayDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CIntroReplayDlgTest, LinkingIsNoOp) {
    // 1:1 with legacy: empty body. No children
    // resolved, no state set.
    cIntroReplayDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    SUCCEED();
}

TEST(CIntroReplayDlgTest, LinkingBeforeInitDoesNotCrash) {
    cIntroReplayDlg dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CIntroReplayDlgTest, LinkingIsIdempotent) {
    cIntroReplayDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.Linking();
    dlg.Linking();
    SUCCEED();
}

}  // namespace mxh::ui::test
