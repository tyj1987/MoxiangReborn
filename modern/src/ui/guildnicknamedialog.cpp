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

void cGuildNickNameDialog::SetCallbacks(
    GetSelectedMemberIdFn getSelectedMemberId,
    GetSelectedMemberNameFn getSelectedMemberName,
    AddSystemMessageFn addSystemMessage,
    GetChatMessageFn getChatMessage,
    void* userData) noexcept {
    m_getSelectedMemberId = getSelectedMemberId;
    m_getSelectedMemberName = getSelectedMemberName;
    m_addSystemMessage = addSystemMessage;
    m_getChatMessage = getChatMessage;
    m_callbackUserData = userData;
}

void cGuildNickNameDialog::SetActive(bool val) noexcept {
    if (val) {
        const auto memberId = m_getSelectedMemberId
            ? m_getSelectedMemberId(m_callbackUserData)
            : 0u;
        if (memberId == 0u) {
            cDialog::SetActive(false);
            if (m_addSystemMessage) {
                m_addSystemMessage(kNoSelectionMessageId, m_callbackUserData);
            }
            return;
        }
        if (m_pNickName) m_pNickName->SetEditText("");
        SetNickMsg(m_getSelectedMemberName
            ? m_getSelectedMemberName(m_callbackUserData)
            : nullptr);
    } else if (m_pNickName) {
        m_pNickName->SetFocusEdit(false);
    }
    cDialog::SetActive(val);
}

void cGuildNickNameDialog::SetNickMsg(const char* name) {
    if (!m_pNickMsg) return;
    const char* format = m_getChatMessage
        ? m_getChatMessage(kNickPromptMessageId, m_callbackUserData)
        : nullptr;
    if (!format) {
        format = name ? "GUILD_NICK_MSG_FORMAT %s" : "GUILD_NICK_MSG_FORMAT";
    }
    char text[128]{};
    std::snprintf(text, sizeof(text), format, name ? name : "");
    m_pNickMsg->SetScriptText(text);
}


}  // namespace mxh::ui
