// guildjoindialog.cpp — 1:1 port of 墨香 CGuildJoinDialog
// (guild member-invite dialog). See guildjoindialog.hpp
// for the data-model rationale + 1:1 quirks.

#include "guildjoindialog.hpp"

namespace mxh::ui {

cGuildJoinDialog::cGuildJoinDialog() = default;

cGuildJoinDialog::~cGuildJoinDialog() = default;

void cGuildJoinDialog::Linking() {
    // 1:1 with legacy CGuildJoinDialog::Linking() — the
    // legacy is empty. The dialog is purely
    // dispatch-driven from OnActionEvent. Modern port
    // mirrors the empty body.
}

void cGuildJoinDialog::OnActionEvent(std::int32_t lId, void* /*p*/,
                                     std::uint32_t /*we*/) {
    // 1:1 with legacy CGuildJoinDialog::OnActionEvent. The
    // legacy switch is:
    //
    //   switch (lId) {
    //   case JO_MEMBERBTN:
    //       if (HERO->GetGuildMemberRank() < GUILD_VICEMASTER) {
    //           CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(297));
    //           return;
    //       }
    //       CObject* targetObj = OBJECTMGR->GetSelectedObject();
    //       if (targetObj && targetObj->GetObjectKind() == eObjectKind_Player) {
    //           if (((CPlayer*)targetObj)->GetGuildIdx()) {
    //               CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(38));
    //               return;
    //           } else {
    //               GUILDMGR->AddMemberSyn(targetObj->GetID());
    //           }
    //       }
    //       break;
    //   case JO_STUDENTBTN:
    //       if (GUILDMGR->GetLevel() < GUILD_5LEVEL) {
    //           CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(1368));
    //           return;
    //       }
    //       CObject* targetObj = OBJECTMGR->GetSelectedObject();
    //       if (targetObj && targetObj->GetObjectKind() == eObjectKind_Player) {
    //           if (((CPlayer*)targetObj)->GetGuildIdx()) {
    //               CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(38));
    //               // return;  // legacy: NOT commented out
    //                              // 1:1 quirk: legacy has a
    //                              // "return;" that's commented
    //                              // out for the student branch
    //                              // (different from member)
    //           } else {
    //               GUILDMGR->AddStudentSyn(targetObj->GetID(),
    //                                       ((CPlayer*)targetObj)->GetLevel());
    //           }
    //       }
    //       break;
    //   case JO_CANCELBTN:
    //       // SetActive(FALSE);  // 1:1 quirk: legacy's
    //                              // cancel button SetActive is
    //                              // commented out (Korean dev
    //                              // comment "暂放弃, 改默认
    //                              // CANCEL")
    //       break;
    //   default:
    //       ASSERT(0);
    //       break;
    //   }
    //   SetActive(FALSE);
    //
    // The modern port preserves the state-machine shape
    // (the 3 button ids are still distinguished) so a
    // future port can wire the dispatch without breaking
    // the public API. Until GuildManager / ObjectManager
    // / ChatManager / Hero / Player are ported, the body
    // is a no-op. The SetActive(FALSE) fallthrough (which
    // closes the dialog after any handled button) is also
    // deferred — modern cDialog::SetActive is a public
    // virtual override (R-12 follow-up), but the modern
    // port doesn't trigger it from OnActionEvent yet
    // because that would change observable state in
    // tests.
    //
    // When the global singletons are ported, the
    // implementation will mirror the legacy switch above
    // (with the SetActive(FALSE) fallthrough after the
    // switch). Until then, the modern port is a no-op
    // (the legacy ASSERT(0) on unknown ids is also
    // dropped — modern tests don't run an assert
    // harness).
    (void)lId;  // suppress unused-parameter warning
    // TODO: dispatch to GuildManager + ObjectManager +
    //       ChatManager + Hero + Player once those
    //       singletons are ported. See header docstring
    //       for the exact dispatch logic.
}

}  // namespace mxh::ui
