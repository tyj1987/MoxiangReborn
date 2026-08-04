// guildinvitationkindselectiondialog.cpp — 1:1 port of 墨香
// CGuildInvitationKindSelectionDialog (guild invitation
// kind selector). See
// guildinvitationkindselectiondialog.hpp for the
// data-model rationale + 1:1 quirks.

#include "guildinvitationkindselectiondialog.hpp"
#include "legacy_window_event.hpp"

namespace mxh::ui {

cGuildInvitationKindSelectionDialog::cGuildInvitationKindSelectionDialog() {
    // 1:1 with legacy ctor: empty body, no state init.
}

cGuildInvitationKindSelectionDialog::~cGuildInvitationKindSelectionDialog() = default;

void cGuildInvitationKindSelectionDialog::SetCallbacks(
    GetSelectedObjectIdFn        getSelectedObjectId,
    IsSelectedObjectPlayerFn     isSelectedObjectPlayer,
    GetSelectedObjectGuildIdxFn  getSelectedObjectGuildIdx,
    GetSelectedObjectLevelFn     getSelectedObjectLevel,
    GetGuildLevelFn              getGuildLevel,
    GetChatMessageFn             getChatMessage,
    AddSystemMessageFn           addSystemMessage,
    AddMemberSynFn               addMemberSyn,
    AddStudentSynFn              addStudentSyn,
    void*                        userData) noexcept {
    m_getSelectedObjectId        = getSelectedObjectId;
    m_isSelectedObjectPlayer     = isSelectedObjectPlayer;
    m_getSelectedObjectGuildIdx  = getSelectedObjectGuildIdx;
    m_getSelectedObjectLevel     = getSelectedObjectLevel;
    m_getGuildLevel              = getGuildLevel;
    m_getChatMessage             = getChatMessage;
    m_addSystemMessage           = addSystemMessage;
    m_addMemberSyn               = addMemberSyn;
    m_addStudentSyn              = addStudentSyn;
    m_callbackUserData           = userData;
}

namespace {

// Helper: returns the chat msg for the given legacy
// id, falling back to a placeholder string when the
// host callback is absent (1:1 with legacy
// CHATMGR->GetChatMsg).
const char* GIK_ResolveChatMessage(
    std::int32_t msgId,
    const mxh::ui::cGuildInvitationKindSelectionDialog& dlg) {
    using Dialog = mxh::ui::cGuildInvitationKindSelectionDialog;
    if (dlg.GetChatMessageForTest() != nullptr) {
        return dlg.GetChatMessageForTest()(msgId, dlg.GetCallbackUserDataForTest());
    }
    switch (msgId) {
        case Dialog::kSysmsgAlreadyInGuild:
            return "GUILD_INVITE_ALREADY_IN_GUILD";
        case Dialog::kSysmsgGuildLevelTooLow:
            return "GUILD_INVITE_LEVEL_TOO_LOW";
        default:
            return "GUILD_INVITE_MSG";
    }
}

}  // namespace

void cGuildInvitationKindSelectionDialog::OnActionEvent(std::int32_t lId, void* p, std::uint32_t we) {
    // 1:1 with legacy CGuildInvitationKindSelectionDialog
    // ::OnActionEvent. The legacy dispatches to
    // OBJECTMGR + CHATMGR + GUILDMGR singletons;
    // the modern port replaces those globals with
    // OPTIONAL host callbacks. When a callback is
    // null, the matching branch is a no-op
    // (matches the legacy "singleton not ported"
    // path).
    (void)p;
    constexpr std::uint32_t kBtnClick = legacy_window_event::kButtonClick;  // legacy cWindow::we WE_BTNCLICK.
    if (!(we & kBtnClick)) {
        return;
    }
    switch (lId) {
        case kIdMemberBtn: {
            // 1:1 with legacy JO_MEMBERBTN branch.
            // Control flow:
            //   1. Get selected object id (0 == no
            //      selection; legacy checks
            //      OBJECTMGR->GetSelectedObject()
            //      != nullptr).
            //   2. If selection valid + player + has
            //      guild -> chat msg 38 + EARLY
            //      RETURN (dialog stays open;
            //      1:1 quirk preserved).
            //   3. Else if selection valid + player +
            //      no guild -> AddMemberSyn.
            //   4. Other paths (no selection /
            //      non-player) silently fall through
            //      to the post-branch SetActive(false).
            if (m_getSelectedObjectId != nullptr) {
                const std::uint32_t targetId =
                    m_getSelectedObjectId(m_callbackUserData);
                if (targetId != 0u &&
                    m_isSelectedObjectPlayer != nullptr &&
                    m_isSelectedObjectPlayer(m_callbackUserData)) {
                    const std::uint32_t guildIdx =
                        (m_getSelectedObjectGuildIdx != nullptr)
                            ? m_getSelectedObjectGuildIdx(m_callbackUserData)
                            : 0u;
                    if (guildIdx != 0u) {
                        if (m_addSystemMessage != nullptr) {
                            m_addSystemMessage(
                                GIK_ResolveChatMessage(kSysmsgAlreadyInGuild, *this),
                                m_callbackUserData);
                        }
                        // 1:1 quirk: legacy return;
                        // after chat msg 38 keeps
                        // the dialog open (no
                        // post-branch SetActive).
                        return;
                    }
                    if (m_addMemberSyn != nullptr) {
                        m_addMemberSyn(targetId, m_callbackUserData);
                    }
                }
            }
            break;
        }
        case kIdStudentBtn: {
            // 1:1 with legacy JO_STUDENTBTN branch.
            // Control flow:
            //   1. If GUILDMGR->GetLevel() < 5 ->
            //      chat msg 1368 + EARLY RETURN
            //      (dialog stays open; 1:1 quirk
            //      preserved).
            //   2. Else, run the same MEMBER-style
            //      selected-object check; if the
            //      target is already in a guild,
            //      emit chat msg 38 (but DO NOT
            //      early-return; the legacy STUDENT
            //      branch falls through to
            //      SetActive(false) here).
            //   3. Else (target is a player with no
                  //      guild), call AddStudentSyn(id, level).
            //   4. Other paths silently close via
            //      the post-branch SetActive(false).
            if (m_getGuildLevel != nullptr &&
                m_getGuildLevel(m_callbackUserData) < kGuildStudentMinLevel) {
                if (m_addSystemMessage != nullptr) {
                    m_addSystemMessage(
                        GIK_ResolveChatMessage(kSysmsgGuildLevelTooLow, *this),
                        m_callbackUserData);
                }
                // 1:1 quirk: legacy return;
                // after chat msg 1368 keeps the
                // dialog open.
                return;
            }
            if (m_getSelectedObjectId != nullptr) {
                const std::uint32_t targetId =
                    m_getSelectedObjectId(m_callbackUserData);
                if (targetId != 0u &&
                    m_isSelectedObjectPlayer != nullptr &&
                    m_isSelectedObjectPlayer(m_callbackUserData)) {
                    const std::uint32_t guildIdx =
                        (m_getSelectedObjectGuildIdx != nullptr)
                            ? m_getSelectedObjectGuildIdx(m_callbackUserData)
                            : 0u;
                    if (guildIdx != 0u) {
                        if (m_addSystemMessage != nullptr) {
                            m_addSystemMessage(
                                GIK_ResolveChatMessage(kSysmsgAlreadyInGuild, *this),
                                m_callbackUserData);
                        }
                    } else if (m_addStudentSyn != nullptr) {
                        const std::uint16_t targetLevel =
                            (m_getSelectedObjectLevel != nullptr)
                                ? m_getSelectedObjectLevel(m_callbackUserData)
                                : 0u;
                        m_addStudentSyn(targetId, targetLevel, m_callbackUserData);
                    }
                }
            }
            break;
        }
        case kIdCancelBtn: {
            // 1:1 quirk: legacy JO_CANCELBTN branch
            // is a complete no-op (Korean dev
            // comment: SetActive(FALSE) was
            // commented out -- "give up, change
            // to default CANCEL"). Modern port
            // keeps the comment-out: the CANCEL
            // branch never closes the dialog.
            // The post-branch SetActive(false) is
            // skipped for CANCEL by `return;`
            // below.
            return;
        }
        default:
            // 1:1 quirk: legacy default branch
            // calls ASSERT(0); modern port treats
            // unknown ids as a safe no-op (no
            // assert in modern test harness).
            return;
    }
    // 1:1 with legacy post-switch SetActive(FALSE):
    // runs for MEMBER + STUDENT default paths
    // (whenever their inner `return;` did not
    // fire). CANCEL + default branches `return`
    // above to skip this, preserving the 1:1
    // quirks (CANCEL does NOT close the
    // dialog).
    SetActive(false);
}


}  // namespace mxh::ui
