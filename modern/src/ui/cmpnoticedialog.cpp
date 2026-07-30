// cmpnoticedialog.cpp -- modern implementation of Moxiang CMPNoticeDialog.

#include "cmpnoticedialog.hpp"

#include "ctextarea.hpp"

namespace mxh::ui {

cMPNoticeDialog::cMPNoticeDialog() = default;

cMPNoticeDialog::~cMPNoticeDialog() = default;

void cMPNoticeDialog::Linking() {
    // 1:1 with legacy CMPNoticeDialog::Linking.
    //   m_pNCaution = (cTextArea *)GetWindowForID(MP_NCAUTION);
    //   m_pNCaution->SetScriptText(CHATMGR->GetChatMsg(667));
    //   m_pNRedCaution = (cTextArea *)GetWindowForID(MP_NREDCAUTION);
    //   m_pNRedCaution->SetScriptText(CHATMGR->GetChatMsg(668));
    // The modern port routes the chatmsg lookup through
    // a host-injected callback; the default returns
    // empty string for unknown ids.
    const ChatMsgCallback cb = m_chatMsgCb ? m_chatMsgCb : &cMPNoticeDialog::DefaultChatMsg;
    if (m_pNCaution) {
        m_pNCaution->SetScriptText(cb(kChatMsgNCaution, m_chatMsgUser));
    }
    if (m_pNRedCaution) {
        m_pNRedCaution->SetScriptText(cb(kChatMsgNRedCaution, m_chatMsgUser));
    }
}

const char* cMPNoticeDialog::DefaultChatMsg(int /*chatMsgId*/, void* /*user*/) {
    // 1:1 with the legacy default for chatmsg 667 / 668
    // -- the .bin chatmsg table ships two short Korean
    // caution strings; the modern port uses an empty
    // string (host supplies the localized text via
    // SetChatMsgCallbackForTest).
    return "";
}

} // namespace mxh::ui
