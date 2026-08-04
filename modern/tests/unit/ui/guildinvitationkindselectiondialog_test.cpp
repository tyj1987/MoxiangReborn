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
#include "legacy_window_event.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace mxh::ui::test {

// ===========================================================================
// Callback fixtures
// ===========================================================================

namespace {

// Tracks every callback invocation.
struct HostCalls {
    int member_count = 0;
    int student_count = 0;
    int system_count = 0;
    int chat_count = 0;
    int guildlevel_count = 0;
    int selid_count = 0;
    int isplayer_count = 0;
    int guildidx_count = 0;
    int level_count = 0;

    std::uint32_t last_member_target = 0;
    std::uint32_t last_student_target = 0;
    std::uint16_t last_student_level = 0;
    std::int32_t  last_chat_msg_id = -1;

    static std::uint32_t GetSelId_Default(void* /*userData*/) { return 0u; }
    static std::uint32_t GetSelId_SomePlayer(void* /*userData*/) { return 4242u; }
    static std::uint32_t GetSelId_OtherPlayer(void* /*userData*/) { return 7777u; }

    static bool IsPlayer_True(void* /*userData*/) { return true; }
    static bool IsPlayer_False(void* /*userData*/) { return false; }

    static std::uint32_t GetGuildIdx_Zero(void* /*userData*/) { return 0u; }
    static std::uint32_t GetGuildIdx_Nonzero(void* /*userData*/) { return 10u; }

    static std::uint16_t GetLevel_30(void* /*userData*/) { return 30; }
    static std::uint16_t GetLevel_45(void* /*userData*/) { return 45; }

    static std::uint8_t GetGuildLevel_Below5(void* userData) {
        auto* hc = static_cast<HostCalls*>(userData);
        ++hc->guildlevel_count;
        return 3;  // < kGuildStudentMinLevel
    }
    static std::uint8_t GetGuildLevel_At5(void* userData) {
        auto* hc = static_cast<HostCalls*>(userData);
        ++hc->guildlevel_count;
        return 5;  // == kGuildStudentMinLevel (boundary)
    }
    static std::uint8_t GetGuildLevel_Above5(void* userData) {
        auto* hc = static_cast<HostCalls*>(userData);
        ++hc->guildlevel_count;
        return 7;
    }

    static const char* GetChatMsg_38(void* /*userData*/) {
        return "PLAYER_ALREADY_IN_A_GUILD";
    }
    static const char* GetChatMsg_1368(void* /*userData*/) {
        return "GUILD_LEVEL_TOO_LOW_FOR_STUDENT";
    }
    static const char* GetChatMsg_DefaultFormat(void* /*userData*/) {
        return "CUSTOM_FALLBACK_FORMAT";
    }

    static void AddSystem(const char* /*text*/, void* userData) {
        auto* hc = static_cast<HostCalls*>(userData);
        ++hc->system_count;
    }

    static void AddMemberSyn(std::uint32_t targetId, void* userData) {
        auto* hc = static_cast<HostCalls*>(userData);
        ++hc->member_count;
        hc->last_member_target = targetId;
    }

    static void AddStudentSyn(std::uint32_t targetId, std::uint16_t level,
                              void* userData) {
        auto* hc = static_cast<HostCalls*>(userData);
        ++hc->student_count;
        hc->last_student_target = targetId;
        hc->last_student_level = level;
    }

    // ChatMessage callback that records the requested msg id.
    static const char* ChatRecorder(std::int32_t msgId, void* userData) {
        auto* hc = static_cast<HostCalls*>(userData);
        ++hc->chat_count;
        hc->last_chat_msg_id = msgId;
        if (msgId == 38) return "ALREADY_IN_GUILD";
        if (msgId == 1368) return "LEVEL_TOO_LOW";
        return "OTHER";
    }
};

// Helper: dialog fully linked and active, ready for OnActionEvent.
struct ActiveDialog {
    mxh::ui::cGuildInvitationKindSelectionDialog dlg;
    ActiveDialog() {
        dlg.Init(0, 0, 400, 400, nullptr, 0);
        dlg.Linking();
        dlg.SetActive(true);
    }
};

}  // namespace

// ===========================================================================
// Construction + state (unchanged from prior batch)
// ===========================================================================

TEST(CGuildInvitationKindSelectionDialogTest, DefaultConstructionIsValid) {
    mxh::ui::cGuildInvitationKindSelectionDialog dlg;
    SUCCEED();
}

TEST(CGuildInvitationKindSelectionDialogTest, InheritsDialogTreeManagement) {
    mxh::ui::cGuildInvitationKindSelectionDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CGuildInvitationKindSelectionDialogTest, IdConstantsAreDistinct) {
    EXPECT_NE(mxh::ui::cGuildInvitationKindSelectionDialog::kIdCancelBtn,
              mxh::ui::cGuildInvitationKindSelectionDialog::kIdMemberBtn);
    EXPECT_NE(mxh::ui::cGuildInvitationKindSelectionDialog::kIdCancelBtn,
              mxh::ui::cGuildInvitationKindSelectionDialog::kIdStudentBtn);
    EXPECT_NE(mxh::ui::cGuildInvitationKindSelectionDialog::kIdMemberBtn,
              mxh::ui::cGuildInvitationKindSelectionDialog::kIdStudentBtn);
}

TEST(CGuildInvitationKindSelectionDialogTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(mxh::ui::cGuildInvitationKindSelectionDialog::kIdCancelBtn, 370);
    EXPECT_EQ(mxh::ui::cGuildInvitationKindSelectionDialog::kIdMemberBtn, 371);
    EXPECT_EQ(mxh::ui::cGuildInvitationKindSelectionDialog::kIdStudentBtn, 372);
}

// ===========================================================================
// Linking (1:1 with legacy empty body)
// ===========================================================================

TEST(CGuildInvitationKindSelectionDialogTest, LinkingIsNoOp) {
    mxh::ui::cGuildInvitationKindSelectionDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    SUCCEED();
}

TEST(CGuildInvitationKindSelectionDialogTest, LinkingBeforeInitDoesNotCrash) {
    mxh::ui::cGuildInvitationKindSelectionDialog dlg;
    dlg.Linking();
    SUCCEED();
}

// ===========================================================================
// OnActionEvent - WE_BTNCLICK gate (1:1 with legacy)
// ===========================================================================

TEST(CGuildInvitationKindSelectionDialogTest, OnActionEventNonBtnClickIsNoOp) {
    mxh::ui::cGuildInvitationKindSelectionDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    constexpr std::uint32_t NOT_BTNCLICK = 0x0000u;
    dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdMemberBtn, nullptr, NOT_BTNCLICK);
    dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdStudentBtn, nullptr, NOT_BTNCLICK);
    dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdCancelBtn, nullptr, NOT_BTNCLICK);
    // All branches gated by WE_BTNCLICK; nothing
    // dispatched. The post-branch SetActive(false)
    // also doesn't fire (gating is before the
    // switch).
    EXPECT_TRUE(dlg.isActive());
}

// ===========================================================================
// OnActionEvent - legacy null-singleton path
// (no callbacks installed = matches legacy
// "GUILDMGR/OBJECTMGR not yet ported" runtime)
// ===========================================================================

TEST(CGuildInvitationKindSelectionDialogTest, OnActionEventMemberBtnNoCallbacksClosesDialog) {
    // With no callbacks installed, MEMBER branch
    // skips the inner dispatch and falls through
    // to the post-branch SetActive(false). 1:1
    // with legacy null-singleton path: dialog
    // closes silently.
    ActiveDialog ad;
    constexpr std::uint32_t WE_BTNCLICK = mxh::ui::legacy_window_event::kButtonClick;
    ad.dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdMemberBtn,
                          nullptr, WE_BTNCLICK);
    EXPECT_FALSE(ad.dlg.isActive());
}

TEST(CGuildInvitationKindSelectionDialogTest, OnActionEventStudentBtnNoCallbacksClosesDialog) {
    // Same as MEMBER no-callback path: STUDENT
    // branch with no GetGuildLevel callback
    // installed skips the early-return gate and
    // falls through to SetActive(false).
    ActiveDialog ad;
    constexpr std::uint32_t WE_BTNCLICK = mxh::ui::legacy_window_event::kButtonClick;
    ad.dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdStudentBtn,
                          nullptr, WE_BTNCLICK);
    EXPECT_FALSE(ad.dlg.isActive());
}

TEST(CGuildInvitationKindSelectionDialogTest, OnActionEventCancelBtnStaysOpenEvenWithSetActiveFallthrough) {
    // 1:1 quirk: legacy CANCEL branch's
    // SetActive(FALSE) is commented out. Modern
    // port keeps the comment-out: the dialog stays
    // open after CANCEL even though the post-switch
    // SetActive(false) IS executed (1:1 quirk
    // preserved by the CANCEL branch's `return;`).
    ActiveDialog ad;
    constexpr std::uint32_t WE_BTNCLICK = mxh::ui::legacy_window_event::kButtonClick;
    ad.dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdCancelBtn,
                          nullptr, WE_BTNCLICK);
    EXPECT_TRUE(ad.dlg.isActive());
}

TEST(CGuildInvitationKindSelectionDialogTest, OnActionEventUnknownIdIsNoOp) {
    // 1:1 quirk: legacy default branch calls
    // ASSERT(0); modern port treats unknown ids as
    // a safe no-op (no assert in modern test
    // harness). Unknown id `return;`s before the
    // post-branch SetActive, so the dialog stays
    // open.
    ActiveDialog ad;
    constexpr std::uint32_t WE_BTNCLICK = mxh::ui::legacy_window_event::kButtonClick;
    ad.dlg.OnActionEvent(/*lId=*/9999, nullptr, WE_BTNCLICK);
    EXPECT_TRUE(ad.dlg.isActive());
}

TEST(CGuildInvitationKindSelectionDialogTest, OnActionEventBeforeInitDoesNotCrash) {
    mxh::ui::cGuildInvitationKindSelectionDialog dlg;
    constexpr std::uint32_t WE_BTNCLICK = mxh::ui::legacy_window_event::kButtonClick;
    dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdMemberBtn, nullptr, WE_BTNCLICK);
    dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdStudentBtn, nullptr, WE_BTNCLICK);
    dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdCancelBtn, nullptr, WE_BTNCLICK);
    SUCCEED();
}

// ===========================================================================
// OnActionEvent - full callback dispatch (Phase C host-callback port)
// ===========================================================================

TEST(CGuildInvitationKindSelectionDialogTest, LegacyMessageConstantsMatchSource) {
    // 1:1 with legacy CHATMGR->GetChatMsg ids +
    // GUILD_5LEVEL + eObjectKind_Player.
    EXPECT_EQ(mxh::ui::cGuildInvitationKindSelectionDialog::kSysmsgAlreadyInGuild, 38);
    EXPECT_EQ(mxh::ui::cGuildInvitationKindSelectionDialog::kSysmsgGuildLevelTooLow, 1368);
    EXPECT_EQ(mxh::ui::cGuildInvitationKindSelectionDialog::kGuildStudentMinLevel, 5u);
    EXPECT_EQ(mxh::ui::cGuildInvitationKindSelectionDialog::kObjectKindPlayer, 1);
}

TEST(CGuildInvitationKindSelectionDialogTest, MemberBtnPlayerWithGuildEmits38AndStaysOpen) {
    // 1:1 quirk: legacy MEMBER branch's
    // `return;` after chat msg 38 keeps the dialog
    // open (no SetActive). Verify: emit msg 38
    // once, dialog stays active, AddMemberSyn NOT
    // called.
    ActiveDialog ad;
    HostCalls hc;
    ad.dlg.SetGetSelectedObjectIdCallbackForTest(&HostCalls::GetSelId_SomePlayer);
    ad.dlg.SetIsSelectedObjectPlayerCallbackForTest(&HostCalls::IsPlayer_True);
    ad.dlg.SetGetSelectedObjectGuildIdxCallbackForTest(&HostCalls::GetGuildIdx_Nonzero);
    ad.dlg.SetChatMessageCallbackForTest(&HostCalls::ChatRecorder);
    ad.dlg.SetSystemMessageCallbackForTest(&HostCalls::AddSystem);
    ad.dlg.SetMemberSynCallbackForTest(&HostCalls::AddMemberSyn);
    ad.dlg.SetCallbackUserDataForTest(&hc);

    constexpr std::uint32_t WE_BTNCLICK = mxh::ui::legacy_window_event::kButtonClick;
    ad.dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdMemberBtn,
                          nullptr, WE_BTNCLICK);

    EXPECT_EQ(hc.system_count, 1);
    EXPECT_EQ(hc.last_chat_msg_id, 38);
    EXPECT_EQ(hc.member_count, 0);   // never dispatched
    EXPECT_TRUE(ad.dlg.isActive());   // stays open
}

TEST(CGuildInvitationKindSelectionDialogTest, MemberBtnPlayerNoGuildDispatchesAddMemberSynAndCloses) {
    // MEMBER branch happy path: selection valid,
    // player with no guild -> AddMemberSyn(id) +
    // post-branch SetActive(false) closes the
    // dialog.
    ActiveDialog ad;
    HostCalls hc;
    ad.dlg.SetGetSelectedObjectIdCallbackForTest(&HostCalls::GetSelId_SomePlayer);
    ad.dlg.SetIsSelectedObjectPlayerCallbackForTest(&HostCalls::IsPlayer_True);
    ad.dlg.SetGetSelectedObjectGuildIdxCallbackForTest(&HostCalls::GetGuildIdx_Zero);
    ad.dlg.SetChatMessageCallbackForTest(&HostCalls::ChatRecorder);
    ad.dlg.SetSystemMessageCallbackForTest(&HostCalls::AddSystem);
    ad.dlg.SetMemberSynCallbackForTest(&HostCalls::AddMemberSyn);
    ad.dlg.SetCallbackUserDataForTest(&hc);

    constexpr std::uint32_t WE_BTNCLICK = mxh::ui::legacy_window_event::kButtonClick;
    ad.dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdMemberBtn,
                          nullptr, WE_BTNCLICK);

    EXPECT_EQ(hc.system_count, 0);                    // no chat msg
    EXPECT_EQ(hc.member_count, 1);                    // dispatched
    EXPECT_EQ(hc.last_member_target, 4242u);          // exact id
    EXPECT_FALSE(ad.dlg.isActive());                  // closed
}

TEST(CGuildInvitationKindSelectionDialogTest, MemberBtnNonPlayerSelectionClosesWithoutDispatch) {
    // Selection valid but not a player: legacy
    // silently closes the dialog without
    // dispatching AddMemberSyn.
    ActiveDialog ad;
    HostCalls hc;
    ad.dlg.SetGetSelectedObjectIdCallbackForTest(&HostCalls::GetSelId_SomePlayer);
    ad.dlg.SetIsSelectedObjectPlayerCallbackForTest(&HostCalls::IsPlayer_False);
    ad.dlg.SetGetSelectedObjectGuildIdxCallbackForTest(&HostCalls::GetGuildIdx_Zero);
    ad.dlg.SetMemberSynCallbackForTest(&HostCalls::AddMemberSyn);
    ad.dlg.SetSystemMessageCallbackForTest(&HostCalls::AddSystem);
    ad.dlg.SetCallbackUserDataForTest(&hc);

    constexpr std::uint32_t WE_BTNCLICK = mxh::ui::legacy_window_event::kButtonClick;
    ad.dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdMemberBtn,
                          nullptr, WE_BTNCLICK);

    EXPECT_EQ(hc.member_count, 0);
    EXPECT_EQ(hc.system_count, 0);
    EXPECT_FALSE(ad.dlg.isActive());
}

TEST(CGuildInvitationKindSelectionDialogTest, MemberBtnNoSelectionId0ClosesWithoutDispatch) {
    // Selection id == 0 matches the legacy
    // `targetObj == nullptr` branch: silently
    // close the dialog without dispatch.
    ActiveDialog ad;
    HostCalls hc;
    ad.dlg.SetGetSelectedObjectIdCallbackForTest(&HostCalls::GetSelId_Default);  // returns 0
    ad.dlg.SetIsSelectedObjectPlayerCallbackForTest(&HostCalls::IsPlayer_True);
    ad.dlg.SetMemberSynCallbackForTest(&HostCalls::AddMemberSyn);
    ad.dlg.SetCallbackUserDataForTest(&hc);

    constexpr std::uint32_t WE_BTNCLICK = mxh::ui::legacy_window_event::kButtonClick;
    ad.dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdMemberBtn,
                          nullptr, WE_BTNCLICK);

    EXPECT_EQ(hc.member_count, 0);
    EXPECT_FALSE(ad.dlg.isActive());
}

TEST(CGuildInvitationKindSelectionDialogTest, MemberBtnNullMemberSynCallbackDoesNotCrash) {
    // Optional AddMemberSyn: when the host
    // callback is null, the dispatch skips the
    // AddMemberSyn call but still closes the
    // dialog. (Matches legacy null-singleton path
    // where GUILDMGR->AddMemberSyn call is just
    // left out -- dialog state is independent of
    // the dispatch.)
    ActiveDialog ad;
    HostCalls hc;
    ad.dlg.SetGetSelectedObjectIdCallbackForTest(&HostCalls::GetSelId_SomePlayer);
    ad.dlg.SetIsSelectedObjectPlayerCallbackForTest(&HostCalls::IsPlayer_True);
    ad.dlg.SetGetSelectedObjectGuildIdxCallbackForTest(&HostCalls::GetGuildIdx_Zero);
    // Do NOT install AddMemberSyn callback.
    ad.dlg.SetCallbackUserDataForTest(&hc);

    constexpr std::uint32_t WE_BTNCLICK = mxh::ui::legacy_window_event::kButtonClick;
    ad.dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdMemberBtn,
                          nullptr, WE_BTNCLICK);

    EXPECT_EQ(hc.member_count, 0);
    EXPECT_FALSE(ad.dlg.isActive());
}

TEST(CGuildInvitationKindSelectionDialogTest, StudentBtnGuildLevelBelow5Emits1368AndStaysOpen) {
    // 1:1 quirk: legacy STUDENT branch's
    // `return;` after chat msg 1368 keeps the
    // dialog open (no SetActive). Verify: emit
    // msg 1368 once, dialog stays active, no
    // further dispatch.
    ActiveDialog ad;
    HostCalls hc;
    ad.dlg.SetGetGuildLevelCallbackForTest(&HostCalls::GetGuildLevel_Below5);
    ad.dlg.SetChatMessageCallbackForTest(&HostCalls::ChatRecorder);
    ad.dlg.SetSystemMessageCallbackForTest(&HostCalls::AddSystem);
    ad.dlg.SetGetSelectedObjectIdCallbackForTest(&HostCalls::GetSelId_SomePlayer);
    ad.dlg.SetIsSelectedObjectPlayerCallbackForTest(&HostCalls::IsPlayer_True);
    ad.dlg.SetStudentSynCallbackForTest(&HostCalls::AddStudentSyn);
    ad.dlg.SetCallbackUserDataForTest(&hc);

    constexpr std::uint32_t WE_BTNCLICK = mxh::ui::legacy_window_event::kButtonClick;
    ad.dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdStudentBtn,
                          nullptr, WE_BTNCLICK);

    EXPECT_EQ(hc.system_count, 1);
    EXPECT_EQ(hc.last_chat_msg_id, 1368);
    EXPECT_EQ(hc.student_count, 0);
    EXPECT_TRUE(ad.dlg.isActive());  // stays open
}

TEST(CGuildInvitationKindSelectionDialogTest, StudentBtnGuildLevelAt5BoundaryProceedsToSelectionCheck) {
    // kGuildStudentMinLevel = 5; legacy gate is
    // `GetLevel() < GUILD_5LEVEL`. At exactly 5,
    // the gate does NOT fire; the dialog proceeds
    // to the selected-object check. With no
    // selection, the dialog closes silently.
    ActiveDialog ad;
    HostCalls hc;
    ad.dlg.SetGetGuildLevelCallbackForTest(&HostCalls::GetGuildLevel_At5);  // == 5
    ad.dlg.SetGetSelectedObjectIdCallbackForTest(&HostCalls::GetSelId_Default);  // 0
    ad.dlg.SetStudentSynCallbackForTest(&HostCalls::AddStudentSyn);
    ad.dlg.SetSystemMessageCallbackForTest(&HostCalls::AddSystem);
    ad.dlg.SetCallbackUserDataForTest(&hc);

    constexpr std::uint32_t WE_BTNCLICK = mxh::ui::legacy_window_event::kButtonClick;
    ad.dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdStudentBtn,
                          nullptr, WE_BTNCLICK);

    EXPECT_EQ(hc.system_count, 0);  // did NOT emit 1368
    EXPECT_EQ(hc.student_count, 0);
    EXPECT_FALSE(ad.dlg.isActive());  // closes (no selection)
}

TEST(CGuildInvitationKindSelectionDialogTest, StudentBtnPlayerNoGuildDispatchesAddStudentSynWithLevel) {
    // STUDENT branch happy path: guild >= 5,
    // selection valid, player with no guild -> 
    // AddStudentSyn(id, level) + post-branch
    // SetActive(false) closes the dialog. Note:
    // the STUDENT branch does NOT early-return
    // on the chat msg 38 path (1:1 with legacy).
    ActiveDialog ad;
    HostCalls hc;
    ad.dlg.SetGetGuildLevelCallbackForTest(&HostCalls::GetGuildLevel_Above5);
    ad.dlg.SetGetSelectedObjectIdCallbackForTest(&HostCalls::GetSelId_SomePlayer);
    ad.dlg.SetIsSelectedObjectPlayerCallbackForTest(&HostCalls::IsPlayer_True);
    ad.dlg.SetGetSelectedObjectGuildIdxCallbackForTest(&HostCalls::GetGuildIdx_Zero);
    ad.dlg.SetGetSelectedObjectLevelCallbackForTest(&HostCalls::GetLevel_45);
    ad.dlg.SetSystemMessageCallbackForTest(&HostCalls::AddSystem);
    ad.dlg.SetStudentSynCallbackForTest(&HostCalls::AddStudentSyn);
    ad.dlg.SetCallbackUserDataForTest(&hc);

    constexpr std::uint32_t WE_BTNCLICK = mxh::ui::legacy_window_event::kButtonClick;
    ad.dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdStudentBtn,
                          nullptr, WE_BTNCLICK);

    EXPECT_EQ(hc.system_count, 0);
    EXPECT_EQ(hc.student_count, 1);
    EXPECT_EQ(hc.last_student_target, 4242u);
    EXPECT_EQ(hc.last_student_level, 45);
    EXPECT_FALSE(ad.dlg.isActive());
}

TEST(CGuildInvitationKindSelectionDialogTest, StudentBtnPlayerWithGuildEmits38AndCloses) {
    // STUDENT branch + selection is a player with
    // a guild: legacy emits chat msg 38 but does
    // NOT early-return (unlike MEMBER). The dialog
    // closes via the post-branch SetActive(false).
    ActiveDialog ad;
    HostCalls hc;
    ad.dlg.SetGetGuildLevelCallbackForTest(&HostCalls::GetGuildLevel_Above5);
    ad.dlg.SetGetSelectedObjectIdCallbackForTest(&HostCalls::GetSelId_SomePlayer);
    ad.dlg.SetIsSelectedObjectPlayerCallbackForTest(&HostCalls::IsPlayer_True);
    ad.dlg.SetGetSelectedObjectGuildIdxCallbackForTest(&HostCalls::GetGuildIdx_Nonzero);
    ad.dlg.SetChatMessageCallbackForTest(&HostCalls::ChatRecorder);
    ad.dlg.SetSystemMessageCallbackForTest(&HostCalls::AddSystem);
    ad.dlg.SetStudentSynCallbackForTest(&HostCalls::AddStudentSyn);
    ad.dlg.SetCallbackUserDataForTest(&hc);

    constexpr std::uint32_t WE_BTNCLICK = mxh::ui::legacy_window_event::kButtonClick;
    ad.dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdStudentBtn,
                          nullptr, WE_BTNCLICK);

    EXPECT_EQ(hc.system_count, 1);
    EXPECT_EQ(hc.last_chat_msg_id, 38);
    EXPECT_EQ(hc.student_count, 0);   // did NOT dispatch
    EXPECT_FALSE(ad.dlg.isActive());  // closes via post-branch
}

TEST(CGuildInvitationKindSelectionDialogTest, SetCallbacksReplacesDispatch) {
    // Two clicks: first with one AddMemberSyn
    // callback, then replace it with another.
    // Verify the second click uses the new
    // callback (last_member_target == 7777, not
    // 4242) and the first callback's counter
    // doesn't increment.
    ActiveDialog ad;
    HostCalls hc1;
    HostCalls hc2;

    ad.dlg.SetGetSelectedObjectIdCallbackForTest(&HostCalls::GetSelId_SomePlayer);
    ad.dlg.SetIsSelectedObjectPlayerCallbackForTest(&HostCalls::IsPlayer_True);
    ad.dlg.SetGetSelectedObjectGuildIdxCallbackForTest(&HostCalls::GetGuildIdx_Zero);

    constexpr std::uint32_t WE_BTNCLICK = mxh::ui::legacy_window_event::kButtonClick;

    // First click: install callback #1.
    ad.dlg.SetMemberSynCallbackForTest(&HostCalls::AddMemberSyn);
    ad.dlg.SetCallbackUserDataForTest(&hc1);
    ad.dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdMemberBtn,
                          nullptr, WE_BTNCLICK);
    EXPECT_EQ(hc1.member_count, 1);
    EXPECT_EQ(hc1.last_member_target, 4242u);

    // Re-init the dialog state.
    ad.dlg.SetActive(true);

    // Second click: install callback #2 + swap userData.
    ad.dlg.SetMemberSynCallbackForTest(&HostCalls::AddMemberSyn);
    ad.dlg.SetCallbackUserDataForTest(&hc2);
    ad.dlg.SetGetSelectedObjectIdCallbackForTest(&HostCalls::GetSelId_OtherPlayer);
    ad.dlg.OnActionEvent(mxh::ui::cGuildInvitationKindSelectionDialog::kIdMemberBtn,
                          nullptr, WE_BTNCLICK);
    EXPECT_EQ(hc2.member_count, 1);
    EXPECT_EQ(hc2.last_member_target, 7777u);
    EXPECT_EQ(hc1.member_count, 1);  // NOT incremented
}



// === Canonical WINDOW_EVENT constants (C-Batch-2.68) ===

TEST(CGuildInvitationKindSelectionDialogTest, UsesCanonicalWindowEventConstants) {
    EXPECT_EQ(cGuildInvitationKindSelectionDialog::kWeBtnClick, mxh::ui::legacy_window_event::kButtonClick);
}

}  // namespace mxh::ui::test
