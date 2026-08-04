// namechangedialog.cpp — 1:1 port of 墨香
// CNameChangeDialog (name change editor). See
// namechangedialog.hpp for the data-model
// rationale + 1:1 quirks.

#include "namechangedialog.hpp"
#include "ceditbox.hpp"

#include <cstring>

namespace mxh::ui {

cNameChangeDialog::cNameChangeDialog() {
    // 1:1 with legacy CNameChangeDialog ctor:
    //   m_type = WT_NAMECHANGE_DLG;
    //   m_dwDBIdx = 0;
    //
    // 1:1 quirk: modern cWindow does not have
    // m_type field (removed in Phase 6 when cWindow
    // was modernized). The ctor body is dropped
    // (m_dwDBIdx uses default member init in the
    // header).
}

cNameChangeDialog::~cNameChangeDialog() = default;

void cNameChangeDialog::Linking() {
    // 1:1 with legacy CNameChangeDialog::Linking.
    // The legacy is:
    //   m_pNameBox = (cEditBox*)GetWindowForID(CH_NAME_CHANGE_EDITBOX);
    //   m_pNameBox->SetValidCheck(VCM_CHARNAME);
    m_pNameBox = static_cast<cEditBox*>(findWindowById(kIdNameBox));
    if (m_pNameBox) {
        m_pNameBox->SetValidCheck(kVcmCharname);
    }
}

void cNameChangeDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CNameChangeDialog::SetActive
    // override. The legacy is:
    //   cDialog::SetActive(val);
    //   if (val)
    //     m_pNameBox->SetEditText("");
    cDialog::SetActive(val);
    if (val && m_pNameBox) {
        // 1:1 quirk: modern SetEditText is a no-op
        // unless InitEditbox was called (m_bInitEdit
        // guard). The test caller must call
        // InitEditbox before this method to make
        // the SetEditText take effect.
        m_pNameBox->SetEditText("");
    }
}

void cNameChangeDialog::SetCallbacks(
    AddSystemMessageFn addSystemMessage,
    GetHeroNameFn getHeroName,
    GetHeroObjectIdFn getHeroObjectId,
    IsInvalidCharIncludedFn isInvalidCharIncluded,
    IsUsableNameFn isUsableName,
    SendNameChangeFn sendNameChange,
    void* userData) noexcept {
    m_addSystemMessage = addSystemMessage;
    m_getHeroName = getHeroName;
    m_getHeroObjectId = getHeroObjectId;
    m_isInvalidCharIncluded = isInvalidCharIncluded;
    m_isUsableName = isUsableName;
    m_sendNameChange = sendNameChange;
    m_callbackUserData = userData;
}

void cNameChangeDialog::NameChangeSyn() {
    if (!m_pNameBox) return;

    const std::string& editText = m_pNameBox->editText();
    const auto length = editText.size();
    if (length == 0) {
        if (m_addSystemMessage) {
            m_addSystemMessage(kEmptyNameMessageId, m_callbackUserData);
        }
        return;
    }
    if (length < 4) {
        if (m_addSystemMessage) {
            m_addSystemMessage(kShortNameMessageId, m_callbackUserData);
        }
        return;
    }
    if (length > kMaxNameLength) return;

    const char* heroName = m_getHeroName
        ? m_getHeroName(m_callbackUserData)
        : nullptr;
    if (heroName && std::strcmp(editText.c_str(), heroName) == 0) return;

    if (m_isInvalidCharIncluded &&
        m_isInvalidCharIncluded(
            reinterpret_cast<const unsigned char*>(editText.c_str()),
            m_callbackUserData)) {
        if (m_addSystemMessage) {
            m_addSystemMessage(kInvalidNameMessageId, m_callbackUserData);
        }
        return;
    }
    if (m_isUsableName && !m_isUsableName(editText.c_str(), m_callbackUserData)) {
        if (m_addSystemMessage) {
            m_addSystemMessage(kInvalidNameMessageId, m_callbackUserData);
        }
        return;
    }
    if (m_dwDBIdx == 0 || !m_getHeroObjectId || !m_sendNameChange) return;

    m_sendNameChange(m_getHeroObjectId(m_callbackUserData), m_dwDBIdx,
                     editText.c_str(), m_callbackUserData);
    SetActive(false);
}

}  // namespace mxh::ui
