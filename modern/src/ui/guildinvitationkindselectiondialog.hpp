// guildinvitationkindselectiondialog.hpp — modern port of 墨香
// CGuildInvitationKindSelectionDialog (guild invitation kind
// selector dialog: 3 button — "invite as member" / "invite as
// student" / "cancel").
//
// 1:1 port of legacy
// `CGuildInvitationKindSelectionDialog` from
//   `墨香【源码】\[Client]MH\GuildInvitationKindSelectionDialog.h`
//   (329 B) and
//   `墨香【源码】\[Client]MH\GuildInvitationKindSelectionDialog.cpp`.
//
// What the legacy does:
//   - Ctor / dtor: empty bodies (no state init).
//   - Linking: empty body. The dialog is purely
//     dispatch-driven from OnActionEvent.
//   - OnActionEvent: 3 button ids:
//     * JO_MEMBERBTN: 3-singleton dispatch
//       (OBJECTMGR->GetSelectedObject → if player
//       and no guild → GUILDMGR->AddMemberSyn).
//     * JO_STUDENTBTN: 3-singleton dispatch
//       (GUILDMGR->GetLevel() < GUILD_5LEVEL → chat
//       msg 1368 + return; else
//       GUILDMGR->AddStudentSyn).
//     * JO_CANCELBTN: SetActive(FALSE) is commented
//       out in the legacy (1:1 quirk: Korean dev
//       comment says "暂放弃，改默认 CANCEL" — modern
//       port keeps the comment-out behavior, no
//       SetActive).
//   - After all 3 branches: SetActive(FALSE) closes
//     the dialog. The default branch in the switch
//     calls ASSERT(0) — the modern port treats
//     unknown ids as a no-op (no assert, since the
//     modern test surface does not run an assert
//     harness).
//
// The modern port covers the dialog structure
// (Linking + OnActionEvent) but does NOT port
// GuildManager / ObjectManager / ChatManager /
// Hero / Player. Those are global singletons
// (R-12.x deferred). OnActionEvent is a no-op that
// preserves the *state-machine shape* of the
// legacy (3-button dispatch, post-branch
// SetActive(FALSE) for MEMBER+STUDENT, no-op for
// CANCEL).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 24th **Tier 2** dialog port (after
// cNameChangeNotifyDlg). The dialog has no service
// dependency on the modern service interface
// (Phase 13) — all state lives in 3 global
// singletons.

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cGuildInvitationKindSelectionDialog : public cDialog {
public:
    cGuildInvitationKindSelectionDialog();
    ~cGuildInvitationKindSelectionDialog() override;

    // ----- 1:1 with legacy CGuildInvitationKindSelectionDialog::Linking -----

    // 1:1 with legacy: empty body. The dialog is
    // purely dispatch-driven from OnActionEvent
    // (no children to resolve).
    void Linking() noexcept {}

    // ----- 1:1 with legacy CGuildInvitationKindSelectionDialog::OnActionEvent -----

    // 1:1 with legacy 3-button dispatch. All 3
    // branches are TODO (3-singleton: OBJECTMGR +
    // CHATMGR + GUILDMGR not ported, R-12.x
    // deferred). After all 3 branches, the legacy
    // calls SetActive(FALSE) — the modern port
    // documents this as TODO (GUILDMGR
    // dispatch).
    //
    // 1:1 quirk: legacy CANCEL branch's
    // SetActive(FALSE) is commented out (Korean dev
    // comment). Modern port keeps the comment-out
    // (the CANCEL branch is a complete no-op).
    //
    // 1:1 quirk: legacy default branch calls
    // ASSERT(0). Modern port treats unknown ids as
    // a no-op (no assert).
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- 1:1 with legacy CHATMGR message IDs -----

    // 1:1 with legacy CHATMGR->GetChatMsg(38) --
    //   "already in a guild" -- for when the
    //   target player is already in a guild.
    static constexpr std::int32_t kSysmsgAlreadyInGuild = 38;

    // 1:1 with legacy CHATMGR->GetChatMsg(1368) --
    //   "guild level too low for student invite"
    //   (gates STUDENT branch when the player
    //   guild is below level 5).
    static constexpr std::int32_t kSysmsgGuildLevelTooLow = 1368;

    // 1:1 with legacy GUILD_5LEVEL (5-level
    // minimum for student invites).
    static constexpr std::uint8_t kGuildStudentMinLevel = 5;

    // 1:1 with legacy eObjectKind_Player. The
    // modern port drops the legacy CObject /
    // CPlayer hierarchy (R-12.x deferred) and
    // instead asks the host whether the
    // selected object is a player.
    static constexpr std::int32_t kObjectKindPlayer = 1;

    // ----- Host callback signatures -----

    // 1:1 with legacy
    //   OBJECTMGR->GetSelectedObject()->GetID()
    // Returns the selected object id, or 0 when
    // no object is selected.
    using GetSelectedObjectIdFn = std::uint32_t (*)(void* userData);

    // 1:1 with legacy
    //   OBJECTMGR->GetSelectedObject()
    //     ->GetObjectKind() == eObjectKind_Player
    // Returns true when the selected object is a
    // player (1:1 with kObjectKindPlayer).
    using IsSelectedObjectPlayerFn = bool (*)(void* userData);

    // 1:1 with legacy
    //   ((CPlayer*)targetObj)->GetGuildIdx()
    // Returns the selected player guild idx,
    // or 0 when no guild.
    using GetSelectedObjectGuildIdxFn = std::uint32_t (*)(void* userData);

    // 1:1 with legacy
    //   ((CPlayer*)targetObj)->GetLevel()
    // Returns the selected player level (used by
    // STUDENT branch AddStudentSyn payload).
    using GetSelectedObjectLevelFn = std::uint16_t (*)(void* userData);

    // 1:1 with legacy GUILDMGR->GetLevel().
    using GetGuildLevelFn = std::uint8_t (*)(void* userData);

    // 1:1 with legacy CHATMGR->GetChatMsg(id).
    using GetChatMessageFn = const char* (*)(std::int32_t msgId, void* userData);

    // 1:1 with legacy
    //   CHATMGR->AddMsg(CTC_SYSMSG, text)
    // Modern port drops the CTC_SYSMSG tag
    // parameter and folds it into the
    // dispatch -- the host knows it is a system
    // message (matches the legacy 2-arg form).
    using AddSystemMessageFn = void (*)(const char* text, void* userData);

    // 1:1 with legacy
    //   GUILDMGR->AddMemberSyn(targetObj->GetID())
    using AddMemberSynFn = void (*)(std::uint32_t targetId, void* userData);

    // 1:1 with legacy
    //   GUILDMGR->AddStudentSyn(targetObj->GetID(),
    //                           targetObj->GetLevel())
    using AddStudentSynFn = void (*)(std::uint32_t targetId, std::uint16_t level, void* userData);

    // Install host callbacks. A null callback falls
    // through to the legacy no-op branch (no
    // dispatch). Pass nullptr to clear all.
    void SetCallbacks(
        GetSelectedObjectIdFn        getSelectedObjectId,
        IsSelectedObjectPlayerFn     isSelectedObjectPlayer,
        GetSelectedObjectGuildIdxFn  getSelectedObjectGuildIdx,
        GetSelectedObjectLevelFn     getSelectedObjectLevel,
        GetGuildLevelFn              getGuildLevel,
        GetChatMessageFn             getChatMessage,
        AddSystemMessageFn           addSystemMessage,
        AddMemberSynFn               addMemberSyn,
        AddStudentSynFn              addStudentSyn,
        void*                        userData = nullptr) noexcept;

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h WINDOW_ID values
    // (JO_CANCELBTN / JO_MEMBERBTN / JO_STUDENTBTN).
    // Local 370-372 — distinct from 200-360 used
    // by previous Tier 2 dialogs.
    static constexpr std::int32_t kIdCancelBtn = 370;
    static constexpr std::int32_t kIdMemberBtn = 371;
    static constexpr std::int32_t kIdStudentBtn = 372;

    // ----- Test-only accessors -----

    // Returns the m_getChatMessage callback
    // pointer (used by the cpp-internal
    // GIK_ResolveChatMessage helper).
    GetChatMessageFn GetChatMessageForTest() const noexcept {
        return m_getChatMessage;
    }

    // Returns the m_callbackUserData pointer
    // so the cpp helper can pass it to the
    // GetChatMessage callback.
    void* GetCallbackUserDataForTest() const noexcept {
        return m_callbackUserData;
    }

    // Direct setter (test only) -- lets each
    // test install one callback at a time
    // without re-typing the full SetCallbacks
    // 9-callback signature.
    void SetMemberSynCallbackForTest(AddMemberSynFn fn) noexcept {
        m_addMemberSyn = fn;
    }
    void SetStudentSynCallbackForTest(AddStudentSynFn fn) noexcept {
        m_addStudentSyn = fn;
    }
    void SetSystemMessageCallbackForTest(AddSystemMessageFn fn) noexcept {
        m_addSystemMessage = fn;
    }
    void SetChatMessageCallbackForTest(GetChatMessageFn fn) noexcept {
        m_getChatMessage = fn;
    }
    void SetGetGuildLevelCallbackForTest(GetGuildLevelFn fn) noexcept {
        m_getGuildLevel = fn;
    }
    void SetGetSelectedObjectIdCallbackForTest(GetSelectedObjectIdFn fn) noexcept {
        m_getSelectedObjectId = fn;
    }
    void SetIsSelectedObjectPlayerCallbackForTest(IsSelectedObjectPlayerFn fn) noexcept {
        m_isSelectedObjectPlayer = fn;
    }
    void SetGetSelectedObjectGuildIdxCallbackForTest(GetSelectedObjectGuildIdxFn fn) noexcept {
        m_getSelectedObjectGuildIdx = fn;
    }
    void SetGetSelectedObjectLevelCallbackForTest(GetSelectedObjectLevelFn fn) noexcept {
        m_getSelectedObjectLevel = fn;
    }
    void SetCallbackUserDataForTest(void* userData) noexcept {
        m_callbackUserData = userData;
    }

private:
    // 1:1 host callback pointers (replaces
    // OBJECTMGR + GUILDMGR + CHATMGR globals,
    // R-12.x deferred).
    GetSelectedObjectIdFn        m_getSelectedObjectId        = nullptr;
    IsSelectedObjectPlayerFn     m_isSelectedObjectPlayer     = nullptr;
    GetSelectedObjectGuildIdxFn  m_getSelectedObjectGuildIdx  = nullptr;
    GetSelectedObjectLevelFn     m_getSelectedObjectLevel     = nullptr;
    GetGuildLevelFn              m_getGuildLevel              = nullptr;
    GetChatMessageFn             m_getChatMessage             = nullptr;
    AddSystemMessageFn           m_addSystemMessage           = nullptr;
    AddMemberSynFn               m_addMemberSyn               = nullptr;
    AddStudentSynFn              m_addStudentSyn              = nullptr;
    void*                        m_callbackUserData           = nullptr;

};

}  // namespace mxh::ui
