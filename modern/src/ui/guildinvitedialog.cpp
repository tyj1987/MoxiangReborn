// guildinvitedialog.cpp — 1:1 port of 墨香
// CGuildInviteDialog (guild invitation display
// dialog). See guildinvitedialog.hpp for the
// data-model rationale + 1:1 quirks.

#include "guildinvitedialog.hpp"
#include "ctextarea.hpp"

#include <cstdio>
#include <string>

namespace mxh::ui {

cGuildInviteDialog::cGuildInviteDialog() {
    // 1:1 with legacy CGuildInviteDialog ctor:
    //   m_type = WT_GUILDINVITEDLG;
    //
    // 1:1 quirk: modern cWindow does not have
    // m_type field (removed in Phase 6 when cWindow
    // was modernized). The ctor body is dropped.
}

cGuildInviteDialog::~cGuildInviteDialog() = default;

void cGuildInviteDialog::Linking() {
    // 1:1 with legacy CGuildInviteDialog::Linking.
    // The legacy is:
    //   m_pInviteMsg = (cTextArea*)GetWindowForID(GD_IINVITE);
    m_pInviteMsg = static_cast<cTextArea*>(findWindowById(kIdInviteText));
}

void cGuildInviteDialog::SetInfo(const char* guildName, const char* masterName, int flgKind) {
    // 1:1 with legacy CGuildInviteDialog::SetInfo.
    // The legacy is:
    //   char text[128];
    //   if (FlgKind == AsMember) {
    //     sprintf(text, CHATMGR->GetChatMsg(45), GuildName, MasterName);
    //   } else {  // AsStudent
    //     sprintf(text, CHATMGR->GetChatMsg(1370), MasterName, GuildName);
    //   }
    //   m_pInviteMsg->SetScriptText(text);
    //
    // The modern port:
    //   - Uses placeholder format strings
    //     "GUILD_INVITE_MSG_MEMBER" (AsMember) /
    //     "GUILD_INVITE_MSG_STUDENT" (AsStudent)
    //     instead of CHATMGR->GetChatMsg(45) /
    //     GetChatMsg(1370). When CHATMGR is ported,
    //     the body becomes the real sprintf.
    //   - Local char[128] buffer matches legacy
    //     buffer size.
    //   - 1:1 quirk: the legacy uses AsMember / AsStudent
    //     enum values (typically 0 / 1). The modern
    //     port uses kFlgMember (= 0) / kFlgStudent
    //     (= 1) constants for self-documentation.
    if (!m_pInviteMsg) {
        return;
    }
    char text[128] = {0};
    if (flgKind == kFlgMember) {
        if (guildName && masterName) {
            std::snprintf(text, sizeof(text),
                         "GUILD_INVITE_MSG_MEMBER %s %s",
                         guildName, masterName);
        }
    } else {
        // kFlgStudent or any other value defaults
        // to the student branch (1:1 with legacy
        // `else` fallthrough).
        if (guildName && masterName) {
            std::snprintf(text, sizeof(text),
                         "GUILD_INVITE_MSG_STUDENT %s %s",
                         masterName, guildName);
        }
    }
    m_pInviteMsg->SetScriptText(text);
}

}  // namespace mxh::ui
