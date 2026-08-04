// chaseinputdialog.cpp — 1:1 port of 墨香 CChaseinputDialog
// (chase input dialog). See chaseinputdialog.hpp for the
// data-model rationale + 1:1 quirks.

#include "chaseinputdialog.hpp"
#include "ceditbox.hpp"

#include <cctype>
#include <cstring>
#include <string>

namespace mxh::ui {

cChaseInputDialog::cChaseInputDialog() = default;

cChaseInputDialog::~cChaseInputDialog() = default;

void cChaseInputDialog::SetCallbacks(GetCurrentTimeFn getCurrentTime,
                                    AddSystemMessageFn addSystemMessage,
                                    GetHeroObjectIdFn getHeroObjectId,
                                    GetHeroObjectNameFn getHeroObjectName,
                                    FilterWordFn filterWord,
                                    IsWantedNameFn isWantedName,
                                    SendChaseSynFn sendChaseSyn,
                                    void* userData) noexcept {
    m_getCurrentTimeFn = getCurrentTime;
    m_addSystemMessageFn = addSystemMessage;
    m_getHeroObjectIdFn = getHeroObjectId;
    m_getHeroObjectNameFn = getHeroObjectName;
    m_filterWordFn = filterWord;
    m_isWantedNameFn = isWantedName;
    m_sendChaseSynFn = sendChaseSyn;
    m_callbackUserData = userData;
}

void cChaseInputDialog::Linking() {
    // 1:1 with legacy CChaseinputDialog::Linking. REAL
    // — resolve cEditBox + SetValidCheck. Defensive
    // null-checks (the legacy unconditionally
    // dereferences m_pEditName in SetValidCheck /
    // SetEditText).
    m_pEditName = static_cast<cEditBox*>(findWindowById(kEditNameId));
    if (m_pEditName) {
        // 1:1 quirk: legacy calls
        //   m_pEditName->SetValidCheck(VCM_CHARNAME)
        // where VCM_CHARNAME = 2 (from cIMEex.h, the
        // character-name validator enum). The modern
        // cEditBox supports 0/1/2/3 modes; closest
        // modern equivalent is mode 2 (alpha only).
        // Modern port uses kVcmCharnameAlias = 2.
        m_pEditName->SetValidCheck(kVcmCharnameAlias);
    }
}

void cChaseInputDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CChaseinputDialog::SetActive.
    // The legacy is:
    //   cDialog::SetActive(val);
    //   if (val) {
    //       m_pEditName->SetEditText("");
    //       m_dwItemIdx = 0;
    //   }
    //
    // Modern port: call base SetActive first, then
    // the if val branch.
    cDialog::SetActive(val);
    if (val) {
        if (m_pEditName) {
            m_pEditName->SetEditText("");
        }
        m_dwItemIdx = 0;
    }
}

void cChaseInputDialog::WantedChaseSyn() {
    // 1:1 with legacy CChaseinputDialog::WantedChaseSyn. The
    // unsigned subtraction intentionally preserves gCurTime wrap-around.
    if (!m_getCurrentTimeFn) {
        return;
    }
    const std::uint32_t currentTime =
        m_getCurrentTimeFn(m_callbackUserData);
    if (currentTime - m_LastChktime < kRateLimitMilliseconds) {
        if (m_addSystemMessageFn) {
            m_addSystemMessageFn(kChatMsgRateLimited,
                                 m_callbackUserData);
        }
        return;
    }

    if (!m_pEditName) {
        return;
    }
    const std::string editText = m_pEditName->editText();
    char wantedName[kMaxNameBufferLength] = {};
    const std::size_t copyLength =
        editText.size() < kMaxNameLength ? editText.size()
                                         : kMaxNameLength;
    std::memcpy(wantedName, editText.data(), copyLength);
    if (wantedName[0] == '\0') {
        return;
    }

    if (m_getHeroObjectNameFn) {
        const char* heroName =
            m_getHeroObjectNameFn(m_callbackUserData);
        if (heroName && std::strcmp(wantedName, heroName) == 0) {
            if (m_addSystemMessageFn) {
                m_addSystemMessageFn(kChatMsgSelfTarget,
                                     m_callbackUserData);
            }
            return;
        }
    }

    char uppercaseName[kMaxNameBufferLength] = {};
    std::memcpy(uppercaseName, wantedName, kMaxNameLength);
    for (std::size_t index = 0; index < kMaxNameLength
         && uppercaseName[index] != '\0'; ++index) {
        uppercaseName[index] = static_cast<char>(
            std::toupper(static_cast<unsigned char>(uppercaseName[index])));
    }
    if (m_filterWordFn
        && m_filterWordFn(uppercaseName, m_callbackUserData)) {
        if (m_addSystemMessageFn) {
            m_addSystemMessageFn(kChatMsgFilteredTarget,
                                 m_callbackUserData);
        }
        return;
    }

    if (m_dwItemIdx == kTrackingJinItemIdx
        && m_isWantedNameFn
        && !m_isWantedNameFn(wantedName, m_callbackUserData)) {
        return;
    }

    if (!m_getHeroObjectIdFn || !m_sendChaseSynFn) {
        return;
    }
    const std::uint32_t objectId =
        m_getHeroObjectIdFn(m_callbackUserData);
    (void)m_sendChaseSynFn(objectId, wantedName, m_dwItemIdx,
                           m_callbackUserData);

    SetActive(false);
    m_LastChktime = currentTime;
}

}  // namespace mxh::ui
