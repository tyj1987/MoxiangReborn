// guildinvitationkindselectiondialog.cpp — 1:1 port of 墨香
// CGuildInvitationKindSelectionDialog (guild invitation
// kind selector). See
// guildinvitationkindselectiondialog.hpp for the
// data-model rationale + 1:1 quirks.

#include "guildinvitationkindselectiondialog.hpp"

namespace mxh::ui {

cGuildInvitationKindSelectionDialog::cGuildInvitationKindSelectionDialog() {
    // 1:1 with legacy ctor: empty body, no state init.
}

cGuildInvitationKindSelectionDialog::~cGuildInvitationKindSelectionDialog() = default;

void cGuildInvitationKindSelectionDialog::OnActionEvent(std::int32_t lId, void* p, std::uint32_t we) {
    // 1:1 with legacy CGuildInvitationKindSelectionDialog
    // ::OnActionEvent. The legacy is:
    //   switch (lId) {
    //   case JO_MEMBERBTN: {
    //     CObject* targetObj = OBJECTMGR->GetSelectedObject();
    //     if (targetObj) {
    //       if (targetObj->GetObjectKind() == eObjectKind_Player) {
    //         if (((CPlayer*)targetObj)->GetGuildIdx()) {
    //           CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(38));
    //           return;
    //         } else
    //           GUILDMGR->AddMemberSyn(targetObj->GetID());
    //       }
    //     }
    //   } break;
    //   case JO_STUDENTBTN: {
    //     if (GUILDMGR->GetLevel() < GUILD_5LEVEL) {
    //       CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(1368));
    //       return;
    //     }
    //     CObject* targetObj = OBJECTMGR->GetSelectedObject();
    //     if (targetObj) {
    //       if (targetObj->GetObjectKind() == eObjectKind_Player) {
    //         if (((CPlayer*)targetObj)->GetGuildIdx()) {
    //           CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(38));
    //         } else
    //           GUILDMGR->AddStudentSyn(targetObj->GetID(), ((CPlayer*)targetObj)->GetLevel());
    //       }
    //     }
    //   } break;
    //   case JO_CANCELBTN: {  // Korean comment: "暂放弃，改默认 CANCEL"
    //     // SetActive(FALSE);  // commented out in legacy
    //   } break;
    //   default: ASSERT(0); break;
    //   }
    //   SetActive(FALSE);
    //
    // The modern port:
    //   - Uses kIdMemberBtn / kIdStudentBtn / kIdCancelBtn
    //     (1:1 with legacy enum).
    //   - WE_BTNCLICK is the click event flag.
    //   - All 3 branches are TODO (3-singleton: OBJECTMGR
    //     + CHATMGR + GUILDMGR not ported, R-12.x
    //     deferred). The legacy control flow (chat msg +
    //     early return vs singleton dispatch) is
    //     documented but not executed.
    //   - 1:1 quirk: legacy CANCEL branch's
    //     SetActive(FALSE) is commented out. Modern port
    //     keeps the comment-out (the CANCEL branch is a
    //     complete no-op, including not closing the
    //     dialog).
    //   - 1:1 quirk: legacy default branch calls
    //     ASSERT(0). Modern port treats unknown ids as
    //     a no-op (no assert, since the modern test
    //     surface does not run an assert harness).
    //   - 1:1 quirk: legacy's `return;` in MEMBER branch
    //     after chat msg 38 means the dialog stays open
    //     (no SetActive(FALSE) on that path). The modern
    //     port preserves this 1:1 behavior: the TODO
    //     documents but does not execute the early return.
    (void)p;
    constexpr std::uint32_t WE_BTNCLICK = 0x0001;  // legacy cWindow::we
    if (!(we & WE_BTNCLICK)) {
        return;
    }
    switch (lId) {
        case kIdMemberBtn: {
            // TODO: 1:1 with legacy JO_MEMBERBTN branch.
            //       3-singleton dispatch
            //       (OBJECTMGR + CHATMGR + GUILDMGR not
            //       ported, R-12.x deferred). The early
            //       `return;` after chat msg 38 (1:1
            //       quirk) is documented but not
            //       executed.
            break;
        }
        case kIdStudentBtn: {
            // TODO: 1:1 with legacy JO_STUDENTBTN branch.
            //       3-singleton dispatch
            //       (GUILDMGR->GetLevel() check +
            //       OBJECTMGR + CHATMGR + GUILDMGR
            //       AddStudentSyn, R-12.x deferred).
            break;
        }
        case kIdCancelBtn: {
            // 1:1 quirk: legacy JO_CANCELBTN branch's
            //   SetActive(FALSE) is commented out
            //   (Korean dev comment: "暂放弃，改默认 CANCEL"
            //   / "gave up, change to default CANCEL").
            //   Modern port keeps the comment-out
            //   (the CANCEL branch is a complete
            //   no-op, including not closing the
            //   dialog).
            break;
        }
        default:
            // 1:1 quirk: legacy default branch calls
            //   ASSERT(0). Modern port treats
            //   unknown ids as a no-op (no assert,
            //   since the modern test surface does
            //   not run an assert harness).
            break;
    }
    // TODO: 1:1 with legacy post-branch SetActive(FALSE)
    //       for MEMBER + STUDENT branches. The
    //       legacy's `return;` in MEMBER branch after
    //       chat msg 38 means the dialog stays open
    //       on that path. When the singletons are
    //       ported, this becomes:
    //         SetActive(false);
    //       placed after the switch (matches legacy
    //       control flow).
}

}  // namespace mxh::ui
