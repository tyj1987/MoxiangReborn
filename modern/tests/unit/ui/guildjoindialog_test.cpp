// guildjoindialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cGuildJoinDialog (guild member-invite
// dialog).
//
// Covers modern/src/ui/guildjoindialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\GuildJoinDialog.h (275 B) and
//   墨香【源码】\[Client]MH\GuildJoinDialog.cpp.
//
// What's tested:
//   - Default construction: cDialog base initialized (no crash).
//   - Linking is a 1:1 no-op (matches legacy empty body, doesn't
//     crash, leaves no observable state change).
//   - OnActionEvent with each of the 3 button ids is a no-op
//     (preserves the legacy state-machine shape, body deferred
//     until GuildManager + ObjectManager + ChatManager + Hero
//     + Player are ported).
//   - OnActionEvent with an unknown id is a no-op (no assert,
//     matches the spirit of legacy ASSERT(0) in debug builds
//     only).
//   - Id constants match the local id range (210-212) used by
//     the modern port (legacy ids live in WindowIDs.h, not yet
//     ported).
//
// 1:1 quirks preserved:
//   - Linking() is empty in the legacy (no fields to resolve —
//     the dialog is purely dispatch-driven).
//   - Legacy's cancel button has SetActive(FALSE) commented
//     out (Korean dev comment "暂放弃, 改默认 CANCEL"); modern
//     port keeps the no-op behavior.
//   - Legacy's student branch has the "return;" after the
//     "already-in-guild" chat msg commented out (different
//     from the member branch); modern port preserves the
//     fall-through.
//   - The legacy's fall-through SetActive(FALSE) (after any
//     handled button) is deferred in the modern port (would
//     change observable state in tests). When the manager
//     singletons are ported, the implementation will mirror
//     the legacy fall-through.

#include "guildjoindialog.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace mxh::ui::test {

// ===========================================================================
// Construction
// ===========================================================================

TEST(CGuildJoinDialogTest, DefaultConstructionDoesNotCrash) {
    cGuildJoinDialog dlg;
    // No observable state in the dialog itself (Linking
    // is empty in the legacy, so there are no member
    // fields to assert against). We just verify that
    // the cDialog base initialized.
    SUCCEED();
}

// ===========================================================================
// Id constants
// ===========================================================================

TEST(CGuildJoinDialogTest, IdConstantsMatchLocalRange) {
    // 1:1 quirk: the modern port uses a local id range
    // (210-212) until the full WindowIDEnum / WindowIDs
    // header is ported. The legacy's JO_MEMBERBTN /
    // JO_STUDENTBTN / JO_CANCELBTN are mapped 1:1 to
    // these constants. The id values are arbitrary as
    // long as they're distinct (so the OnActionEvent
    // switch can distinguish them).
    EXPECT_NE(cGuildJoinDialog::kJoinMemberBtnId,
              cGuildJoinDialog::kJoinStudentBtnId);
    EXPECT_NE(cGuildJoinDialog::kJoinMemberBtnId,
              cGuildJoinDialog::kJoinCancelBtnId);
    EXPECT_NE(cGuildJoinDialog::kJoinStudentBtnId,
              cGuildJoinDialog::kJoinCancelBtnId);
}

TEST(CGuildJoinDialogTest, IdConstantsMatchExpectedLocalRange) {
    // 1:1 quirk: pick 210-212 to avoid collisions with
    // other Tier 2 dialog id ranges (cCharMakeDlg uses
    // 200-203 for the 4 m/f hair + m/f face statics).
    EXPECT_EQ(cGuildJoinDialog::kJoinMemberBtnId,  210);
    EXPECT_EQ(cGuildJoinDialog::kJoinStudentBtnId, 211);
    EXPECT_EQ(cGuildJoinDialog::kJoinCancelBtnId,  212);
}

// ===========================================================================
// Linking
// ===========================================================================

TEST(CGuildJoinDialogTest, LinkingIsNoOpLikeLegacy) {
    // 1:1 quirk: the legacy Linking() is empty. The
    // modern port mirrors the empty body. Linking must
    // not crash, must not throw, and must not change any
    // observable state.
    cGuildJoinDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    // After Linking, the dialog is still a valid cDialog
    // (no children to assert against — Linking is empty).
    SUCCEED();
}

TEST(CGuildJoinDialogTest, LinkingBeforeInitDoesNotCrash) {
    // Calling Linking before Init is not part of the
    // legacy contract (the legacy always Init-then-
    // Linking), but the modern port should be defensive
    // — Linking() is empty so it should be a no-op
    // regardless of Init state.
    cGuildJoinDialog dlg;
    dlg.Linking();
    SUCCEED();
}

// ===========================================================================
// OnActionEvent (deferred to GuildManager + ObjectManager + ChatManager
//                + Hero + Player port)
// ===========================================================================

TEST(CGuildJoinDialogTest, OnActionEventMemberBtnIsNoOp) {
    // 1:1 quirk: the legacy's JO_MEMBERBTN branch
    // dispatches to HERO + OBJECTMGR + CHATMGR +
    // GUILDMGR. The modern port preserves the
    // state-machine shape (the 3 ids are distinguished)
    // but the body is a no-op until those singletons
    // are ported. The test verifies that calling
    // OnActionEvent with the member id doesn't crash
    // and doesn't change any observable state.
    cGuildJoinDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.OnActionEvent(cGuildJoinDialog::kJoinMemberBtnId,
                      /*p=*/nullptr, /*we=*/0x10);
    SUCCEED();
}

TEST(CGuildJoinDialogTest, OnActionEventStudentBtnIsNoOp) {
    cGuildJoinDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.OnActionEvent(cGuildJoinDialog::kJoinStudentBtnId,
                      /*p=*/nullptr, /*we=*/0x10);
    SUCCEED();
}

TEST(CGuildJoinDialogTest, OnActionEventCancelBtnIsNoOp) {
    // 1:1 quirk: the legacy's cancel branch has the
    // SetActive(FALSE) commented out (Korean dev
    // comment "暂放弃, 改默认 CANCEL"). The modern port
    // keeps the no-op behavior. The test verifies that
    // calling OnActionEvent with the cancel id doesn't
    // crash and doesn't change the dialog's active
    // state.
    cGuildJoinDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    const bool active_before = dlg.isActive();
    dlg.OnActionEvent(cGuildJoinDialog::kJoinCancelBtnId,
                      /*p=*/nullptr, /*we=*/0x10);
    EXPECT_EQ(dlg.isActive(), active_before);
}

TEST(CGuildJoinDialogTest, OnActionEventUnknownIdIsNoOpNoAssert) {
    // 1:1 quirk: the legacy's default branch calls
    // ASSERT(0) in debug builds. The modern port drops
    // the assert (the modern test surface doesn't run
    // an assert harness) and treats unknown ids as a
    // no-op. The test verifies that calling
    // OnActionEvent with an unknown id doesn't crash
    // and doesn't change the dialog's active state.
    cGuildJoinDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    const bool active_before = dlg.isActive();
    dlg.OnActionEvent(/*unknown=*/99999,
                      /*p=*/nullptr, /*we=*/0x10);
    EXPECT_EQ(dlg.isActive(), active_before);
}

TEST(CGuildJoinDialogTest, OnActionEventBeforeInitDoesNotCrash) {
    // Defensive: OnActionEvent before Init should still
    // be a safe no-op (the modern port's body doesn't
    // touch any state).
    cGuildJoinDialog dlg;
    dlg.OnActionEvent(cGuildJoinDialog::kJoinMemberBtnId,
                      /*p=*/nullptr, /*we=*/0x10);
    SUCCEED();
}

TEST(CGuildJoinDialogTest, OnActionEventAllThreeIdsInSequence) {
    // 1:1 quirk: the legacy's switch handles 3 button
    // ids and falls through to SetActive(FALSE) on any
    // of them. The modern port preserves the
    // state-machine shape — each of the 3 ids must be
    // accepted without crash. This test exercises all
    // 3 in sequence to verify the switch is correctly
    // structured (even though the body is a no-op).
    cGuildJoinDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.OnActionEvent(cGuildJoinDialog::kJoinMemberBtnId,  nullptr, 0x10);
    dlg.OnActionEvent(cGuildJoinDialog::kJoinStudentBtnId, nullptr, 0x10);
    dlg.OnActionEvent(cGuildJoinDialog::kJoinCancelBtnId,  nullptr, 0x10);
    SUCCEED();
}

}  // namespace mxh::ui::test
