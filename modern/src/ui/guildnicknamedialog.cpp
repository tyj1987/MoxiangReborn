// guildnicknamedialog.cpp — 1:1 port of 墨香
// CGuildNickNameDialog (guild member nickname
// editor). See guildnicknamedialog.hpp for the
// data-model rationale + 1:1 quirks.

#include "guildnicknamedialog.hpp"
#include "ctextarea.hpp"
#include "ceditbox.hpp"

#include <cstdio>
#include <string>

namespace mxh::ui {

cGuildNickNameDialog::cGuildNickNameDialog() {
    // 1:1 with legacy CGuildNickNameDialog ctor:
    //   m_type = WT_GUILDNICKNAMEDLG;
    //
    // 1:1 quirk: modern cWindow does not have
    // m_type field (removed in Phase 6 when cWindow
    // was modernized). The ctor body is dropped.
}

cGuildNickNameDialog::~cGuildNickNameDialog() = default;

void cGuildNickNameDialog::Linking() {
    // 1:1 with legacy CGuildNickNameDialog::Linking.
    // The legacy is:
    //   m_pNickMsg = (cTextArea*)GetWindowForID(GD_NICKTEXTAREA);
    //   m_pNickName = (cEditBox*)GetWindowForID(GD_NICKNAMEEDIT);
    //   m_pNickName->SetValidCheck(VCM_SPACE);
    m_pNickMsg = static_cast<cTextArea*>(findWindowById(kIdNickTextArea));
    m_pNickName = static_cast<cEditBox*>(findWindowById(kIdNickNameEdit));
    if (m_pNickName) {
        m_pNickName->SetValidCheck(kVcmSpace);
    }
}

void cGuildNickNameDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CGuildNickNameDialog::SetActive
    // override. The legacy is:
    //   if (val == TRUE) {
    //     if (GUILDMGR->GetSelectedMemberID() == 0) {
    //       cDialog::SetActive(FALSE);
    //       CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(714));
    //       return;
    //     }
    //     m_pNickName->SetEditText("");
    //     SetNickMsg(GUILDMGR->GetSelectedMemberName());
    //   } else {
    //     m_pNickName->SetFocusEdit(FALSE);
    //   }
    //   cDialog::SetActive(val);
    //
    // The modern port:
    //   - The GUILDMGR check + CHATMGR dispatch +
    //     SetEditText("") is TODO (GUILDMGR +
    //     CHATMGR not ported, R-12.x deferred).
    //   - The val == false path (SetFocusEdit(false))
    //     is REAL (no singleton dep).
    //   - Always calls base SetActive(val) (matches
    //     legacy call order).
    if (val) {
        // TODO: 1:1 with legacy val == TRUE path:
        //   if (GUILDMGR->GetSelectedMemberID() == 0) {
        //     cDialog::SetActive(false);
        //     CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(714));
        //     return;
        //   }
        //   if (m_pNickName) m_pNickName->SetEditText("");
        //   SetNickMsg(GUILDMGR->GetSelectedMemberName());
        //
        // GUILDMGR + CHATMGR not ported (R-12.x
        // deferred). When ported, the body becomes
        // the legacy code.
    } else {
        if (m_pNickName) {
            m_pNickName->SetFocusEdit(false);
        }
    }
    cDialog::SetActive(val);
}

void cGuildNickNameDialog::SetNickMsg(const char* name) {
    // 1:1 with legacy CGuildNickNameDialog::SetNickMsg.
    // The legacy is:
    //   char text[128];
    //   sprintf(text, CHATMGR->GetChatMsg(704), Name);
    //   m_pNickMsg->SetScriptText(text);
    //
    // The modern port:
    //   - Uses placeholder format string
    //     "GUILD_NICK_MSG_FORMAT" instead of
    //     CHATMGR->GetChatMsg(704). When CHATMGR
    //     is ported, the body becomes the real
    //     sprintf with CHATMGR->GetChatMsg(704).
    //   - Local char[128] buffer matches legacy
    //     buffer size.
    if (!m_pNickMsg) {
        return;
    }
    char text[128] = {0};
    if (name) {
        std::snprintf(text, sizeof(text), "GUILD_NICK_MSG_FORMAT %s", name);
    } else {
        std::snprintf(text, sizeof(text), "GUILD_NICK_MSG_FORMAT");
    }
    m_pNickMsg->SetScriptText(text);
}

}  // namespace mxh::ui
