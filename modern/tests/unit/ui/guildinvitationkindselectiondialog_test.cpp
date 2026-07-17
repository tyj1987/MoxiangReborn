// guildinvitationkindselectiondialog_test.cpp - Phase 12.x P2-12
// Tier 2 dialog 1:1 port contract test for modern
// cGuildInvitationKindSelectionDialog (guild invitation kind
// selector dialog: 3 button — "invite as member" /
// "invite as student" / "cancel").
//
// Covers modern/src/ui/guildinvitationkindselectiondialog.
// {hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\GuildInvitationKindSelectionDialog.h
//   (329 B) and
//   墨香【源码】\[Client]MH\GuildInvitationKindSelectionDialog.cpp.
//
// What's tested:
//   - Default construction: cGuildInvitationKindSelectionDialog
//     is a cDialog and inherits its tree management.
//   - 3 id constants are distinct (1:1 with legacy
//     JO_MEMBERBTN / JO_STUDENTBTN / JO_CANCELBTN).
//   - 3 id constants match expected local range
//     370-372 (no collision with previous Tier 2
//     dialogs 200-360).
//   - Linking is a no-op (1:1 with legacy empty body).
//   - OnActionEvent is gated by WE_BTNCLICK (1:1
//     with legacy `if (we & WE_BTNCLICK)`).
//   - OnActionEvent MEMBER + STUDENT branches are
//     TODO (3-singleton dispatch: OBJECTMGR +
//     CHATMGR + GUILDMGR, R-12.x deferred).
//   - OnActionEvent CANCEL branch is a no-op (1:1
//     quirk: legacy SetActive(FALSE) is commented
//     out).
//   - OnActionEvent unknown id is a safe no-op
//     (1:1 quirk: legacy ASSERT(0), modern skips
//     assert).
//   - OnActionEvent non-BTNCLICK is a safe no-op
//     (1:1 with legacy gate).
//   - OnActionEvent before Init does not crash.
//
// 1:1 quirks preserved:
//   - Ctor / dtor body empty.
//   - Linking body empty.
//   - 1:1 quirk: legacy CANCEL branch's
//     SetActive(FALSE) is commented out (Korean dev
//     comment). Modern port keeps the comment-out.
//   - 1:1 quirk: legacy default branch calls
//     ASSERT(0). Modern port treats unknown ids as
//     no-op.
//   - 1:1 quirk: legacy's `return;` in MEMBER branch
//     after chat msg 38 means the dialog stays open
//     (no SetActive(FALSE) on that path). The
//     modern port preserves this 1:1 behavior: the
//     TODO documents but does not execute the early
//     return.
//   - Local id range 370-372 (distinct from
//     200-360 used by previous Tier 2 dialogs; no
//     collision).

#include "guildinvitationkindselectiondialog.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace mxh::ui::test {

// ===========================================================================
// Construction + state
// ===========================================================================

TEST(CGuildInvitationKindSelectionDialogTest, DefaultConstructionIsValid) {
    cGuildInvitationKindSelectionDialog dlg;
    SUCCEED();
}

TEST(CGuildInvitationKindSelectionDialogTest, InheritsDialogTreeManagement) {
    cGuildInvitationKindSelectionDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CGuildInvitationKindSelectionDialogTest, IdConstantsAreDistinct) {
    EXPECT_NE(cGuildInvitationKindSelectionDialog::kIdCancelBtn,
              cGuildInvitationKindSelectionDialog::kIdMemberBtn);
    EXPECT_NE(cGuildInvitationKindSelectionDialog::kIdCancelBtn,
              cGuildInvitationKindSelectionDialog::kIdStudentBtn);
    EXPECT_NE(cGuildInvitationKindSelectionDialog::kIdMemberBtn,
              cGuildInvitationKindSelectionDialog::kIdStudentBtn);
}

TEST(CGuildInvitationKindSelectionDialogTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cGuildInvitationKindSelectionDialog::kIdCancelBtn, 370);
    EXPECT_EQ(cGuildInvitationKindSelectionDialog::kIdMemberBtn, 371);
    EXPECT_EQ(cGuildInvitationKindSelectionDialog::kIdStudentBtn, 372);
}

// ===========================================================================
// Linking
// ===========================================================================

TEST(CGuildInvitationKindSelectionDialogTest, LinkingIsNoOp) {
    cGuildInvitationKindSelectionDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    SUCCEED();
}

TEST(CGuildInvitationKindSelectionDialogTest, LinkingBeforeInitDoesNotCrash) {
    cGuildInvitationKindSelectionDialog dlg;
    dlg.Linking();
    SUCCEED();
}

// ===========================================================================
// OnActionEvent
// ===========================================================================

TEST(CGuildInvitationKindSelectionDialogTest, OnActionEventNonBtnClickIsNoOp) {
    cGuildInvitationKindSelectionDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    constexpr std::uint32_t NOT_BTNCLICK = 0x0000;
    dlg.OnActionEvent(cGuildInvitationKindSelectionDialog::kIdMemberBtn, nullptr, NOT_BTNCLICK);
    dlg.OnActionEvent(cGuildInvitationKindSelectionDialog::kIdStudentBtn, nullptr, NOT_BTNCLICK);
    dlg.OnActionEvent(cGuildInvitationKindSelectionDialog::kIdCancelBtn, nullptr, NOT_BTNCLICK);
    // All branches gated by WE_BTNCLICK; nothing
    // dispatched.
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildInvitationKindSelectionDialogTest, OnActionEventMemberBtnIsNoOpUntilGuildMgrPort) {
    cGuildInvitationKindSelectionDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    constexpr std::uint32_t WE_BTNCLICK = 0x0001;
    dlg.OnActionEvent(cGuildInvitationKindSelectionDialog::kIdMemberBtn, nullptr, WE_BTNCLICK);
    // TODO: 3-singleton dispatch (R-12.x deferred).
    // Dialog still active (no post-branch SetActive(FALSE)
    // yet, and MEMBER's early-return path would keep
    // it open).
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildInvitationKindSelectionDialogTest, OnActionEventStudentBtnIsNoOpUntilGuildMgrPort) {
    cGuildInvitationKindSelectionDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    constexpr std::uint32_t WE_BTNCLICK = 0x0001;
    dlg.OnActionEvent(cGuildInvitationKindSelectionDialog::kIdStudentBtn, nullptr, WE_BTNCLICK);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildInvitationKindSelectionDialogTest, OnActionEventCancelBtnIsNoOpKeepsDialogOpen) {
    // 1:1 quirk: legacy CANCEL branch's
    // SetActive(FALSE) is commented out. Modern port
    // keeps the comment-out: the dialog stays
    // open after CANCEL.
    cGuildInvitationKindSelectionDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    constexpr std::uint32_t WE_BTNCLICK = 0x0001;
    dlg.OnActionEvent(cGuildInvitationKindSelectionDialog::kIdCancelBtn, nullptr, WE_BTNCLICK);
    // Dialog stays open (1:1 quirk preserved).
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildInvitationKindSelectionDialogTest, OnActionEventUnknownIdIsNoOp) {
    cGuildInvitationKindSelectionDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    constexpr std::uint32_t WE_BTNCLICK = 0x0001;
    // 1:1 quirk: legacy default branch calls
    // ASSERT(0); modern port treats unknown ids as
    // a safe no-op.
    dlg.OnActionEvent(/*lId=*/9999, nullptr, WE_BTNCLICK);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildInvitationKindSelectionDialogTest, OnActionEventAllThreeIdsInSequence) {
    cGuildInvitationKindSelectionDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    constexpr std::uint32_t WE_BTNCLICK = 0x0001;
    dlg.OnActionEvent(cGuildInvitationKindSelectionDialog::kIdMemberBtn, nullptr, WE_BTNCLICK);
    dlg.OnActionEvent(cGuildInvitationKindSelectionDialog::kIdStudentBtn, nullptr, WE_BTNCLICK);
    dlg.OnActionEvent(cGuildInvitationKindSelectionDialog::kIdCancelBtn, nullptr, WE_BTNCLICK);
    // All 3 branches TODO; dialog still active.
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildInvitationKindSelectionDialogTest, OnActionEventBeforeInitDoesNotCrash) {
    cGuildInvitationKindSelectionDialog dlg;
    constexpr std::uint32_t WE_BTNCLICK = 0x0001;
    dlg.OnActionEvent(cGuildInvitationKindSelectionDialog::kIdMemberBtn, nullptr, WE_BTNCLICK);
    dlg.OnActionEvent(cGuildInvitationKindSelectionDialog::kIdStudentBtn, nullptr, WE_BTNCLICK);
    dlg.OnActionEvent(cGuildInvitationKindSelectionDialog::kIdCancelBtn, nullptr, WE_BTNCLICK);
    SUCCEED();
}

}  // namespace mxh::ui::test
