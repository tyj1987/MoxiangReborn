// cguildinvitedialog.cpp -- modern implementation of
//   Moxiang CGuildInviteDialog.

#include "cguildinvitedialog.hpp"

#include "ctextarea.hpp"

#include <cstdio>

namespace mxh::ui {

cGuildInviteDialog::cGuildInviteDialog() = default;

cGuildInviteDialog::~cGuildInviteDialog() = default;

void cGuildInviteDialog::Linking() {
    // 1:1 with legacy CGuildInviteDialog::Linking.
    //   m_pInviteMsg = (cTextArea*)GetWindowForID(GD_IINVITE);
    // The modern port lets the host inject the cTextArea
    // pointer via SetInviteTextForTest (called before
    // Linking).
}

void cGuildInviteDialog::SetInfo(const char* guildName, const char* masterName, int flgKind) {
    // 1:1 with legacy CGuildInviteDialog::SetInfo.
    //   if (FlgKind == AsMember) {
    //       sprintf(text, CHATMGR->GetChatMsg(45),
    //               GuildName, MasterName);
    //   } else {  // AsStudent
    //       sprintf(text, CHATMGR->GetChatMsg(1370),
    //               MasterName, GuildName);
    //   }
    //   m_pInviteMsg->SetScriptText(text);
    if (!m_pInviteMsg) return;
    if (!guildName || !masterName) return;  // 1:1 quirk guard

    const ChatMsgCallback cb = m_chatMsgCb ? m_chatMsgCb : &cGuildInviteDialog::DefaultChatMsg;
    char text[128] = {0};
    if (flgKind == kFlgMember) {
        std::snprintf(text, sizeof(text),
                      cb(kChatMsgMember, m_chatMsgUser),
                      guildName, masterName);
    } else {
        // kFlgStudent (or any other value) defaults to
        // the student branch (1:1 with legacy `else`
        // fallthrough).
        std::snprintf(text, sizeof(text),
                      cb(kChatMsgStudent, m_chatMsgUser),
                      masterName, guildName);
    }
    m_pInviteMsg->SetScriptText(text);
}

const char* cGuildInviteDialog::DefaultChatMsg(int chatMsgId, void* /*user*/) {
    // 1:1 with the legacy default for chatmsg 45 /
    // 1370.  The legacy .bin chatmsg table ships two
    // short Korean invitation strings; the modern
    // port uses the legacy default %s %s placeholders
    // (1:1 with the Korean / Chinese text from the
    // default resource).  The host can override via
    // SetChatMsgCallbackForTest to use the localized
    // strings.
    if (chatMsgId == kChatMsgMember)  return "%s %s";
    if (chatMsgId == kChatMsgStudent) return "%s %s";
    return "";
}

} // namespace mxh::ui
