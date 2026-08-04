// shoutdialog.cpp — 1:1 port of 墨香 CShoutDialog
// (shout message sender dialog). See shoutdialog.hpp
// for the data-model rationale + 1:1 quirks.

#include "shoutdialog.hpp"
#include "ceditbox.hpp"

#include <cstdio>

namespace mxh::ui {

cShoutDialog::cShoutDialog() {
    // 1:1 with legacy CShoutDialog ctor:
    //   m_type = WT_SHOUT_DLG;
    //   m_dwItemIdx = m_dwItemPos = 0;
    //
    // 1:1 quirk: modern cWindow does not have
    // m_type field (removed in Phase 6 when cWindow
    // was modernized). The ctor body is dropped
    // (m_dwItemIdx / m_dwItemPos use default
    // member init in the header).
}

cShoutDialog::~cShoutDialog() = default;

void cShoutDialog::Linking() {
    // 1:1 with legacy CShoutDialog::Linking. The
    // legacy is:
    //   m_pMsgBox = (cEditBox*)GetWindowForID(CHA_MSG);
    m_pMsgBox = static_cast<cEditBox*>(findWindowById(kIdMsgBox));
}

void cShoutDialog::SetItemInfo(std::uint32_t itemIdx, std::uint32_t itemPos) noexcept {
    // 1:1 with legacy CShoutDialog::SetItemInfo
    // (inline setter in the header).
    m_dwItemIdx = itemIdx;
    m_dwItemPos = itemPos;
}

void cShoutDialog::SetCallbacks(
    AddSystemMessageFn addSystemMessage,
    FilterChatFn filterChat,
    GetHeroNameFn getHeroName,
    GetHeroObjectIdFn getHeroObjectId,
    SendShoutFn sendShout,
    void* userData) noexcept {
    m_addSystemMessage = addSystemMessage;
    m_filterChat = filterChat;
    m_getHeroName = getHeroName;
    m_getHeroObjectId = getHeroObjectId;
    m_sendShout = sendShout;
    m_callbackUserData = userData;
}

bool cShoutDialog::SendShoutMsgSyn() {
    if (!m_pMsgBox) return false;

    const std::string message = m_pMsgBox->editText();
    if (message.empty()) {
        if (m_addSystemMessage) {
            m_addSystemMessage(kEmptyMessageId, m_callbackUserData);
        }
        return false;
    }

    m_pMsgBox->SetEditText("");
    if (m_filterChat && m_filterChat(message.c_str(), m_callbackUserData)) {
        if (m_addSystemMessage) {
            m_addSystemMessage(kFilteredMessageId, m_callbackUserData);
        }
        return false;
    }
    if (!m_getHeroName || !m_getHeroObjectId || !m_sendShout) return false;

    const char* heroName = m_getHeroName(m_callbackUserData);
    if (!heroName) return false;

    char formatted[kMaxShoutLength + 1]{};
    std::snprintf(formatted, sizeof(formatted), "%s : %s",
                  heroName, message.c_str());
    m_sendShout(m_getHeroObjectId(m_callbackUserData),
                static_cast<std::uint16_t>(m_dwItemIdx),
                static_cast<std::uint16_t>(m_dwItemPos),
                formatted, m_callbackUserData);

    SetActive(false);
    m_dwItemIdx = 0;
    m_dwItemPos = 0;
    return true;
}

}  // namespace mxh::ui
