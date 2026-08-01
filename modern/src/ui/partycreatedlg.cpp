#include "partycreatedlg.hpp"

#include "mxh/ui/cButton.hpp"
#include "mxh/ui/cCheckBox.hpp"
#include "mxh/ui/cComboBox.hpp"
#include "mxh/ui/cEditBox.hpp"

#include <cstdlib>
#include <cstring>

namespace mxh::ui {

cPartyCreateDlg::cPartyCreateDlg() = default;
cPartyCreateDlg::~cPartyCreateDlg() = default;

void cPartyCreateDlg::SetCallbacks(CreateSynFn createSyn, ChatMessageFn chatMsg,
                                   ResourceMsgFn resourceMsg,
                                   HasPartyFn hasParty,
                                   void* userData) noexcept {
    m_createSynFn = createSyn;
    m_chatMsgFn = chatMsg;
    m_resourceMsgFn = resourceMsg;
    m_hasPartyFn = hasParty;
    m_callbackUserData = userData;
}

void cPartyCreateDlg::SetControlsForTest(cEditBox* theme, cEditBox* minLevel,
                                         cEditBox* maxLevel, cCheckBox* publicCheck,
                                         cCheckBox* privateCheck, cComboBox* distribute,
                                         cComboBox* memberNum, cButton* okBtn,
                                         cButton* cancelBtn) noexcept {
    m_pThemeEdit = theme;
    m_pMinLevelEdit = minLevel;
    m_pMaxLevelEdit = maxLevel;
    m_pPublicCheck = publicCheck;
    m_pPrivateCheck = privateCheck;
    m_pDistribute = distribute;
    m_pMemberNumCombo = memberNum;
    m_pOKBtn = okBtn;
    m_pCancelBtn = cancelBtn;
}

void cPartyCreateDlg::DispatchChatMessage(std::int32_t messageId) {
    if (m_chatMsgFn) {
        (void)m_chatMsgFn(messageId, m_callbackUserData);
    }
}

PartyDivisionOption cPartyCreateDlg::ResolveDivisionOption(
    const char* text) const {
    if (m_resourceMsgFn) {
        const char* randomFmt = m_resourceMsgFn(kResourceRandomOption,
                                                m_callbackUserData);
        if (randomFmt && text && std::strcmp(text, randomFmt) == 0) {
            return PartyDivisionOption::Random;
        }
        const char* damageFmt = m_resourceMsgFn(kResourceDamageOption,
                                                 m_callbackUserData);
        if (damageFmt && text && std::strcmp(text, damageFmt) == 0) {
            return PartyDivisionOption::Damage;
        }
    }
    return PartyDivisionOption::Unknown;
}

void cPartyCreateDlg::ApplyOptionDefaults() {
    if (m_pThemeEdit) {
        m_pThemeEdit->SetEditText("");
    }
    if (m_pPublicCheck) {
        m_pPublicCheck->SetChecked(true);
    }
    if (m_pPrivateCheck) {
        m_pPrivateCheck->SetChecked(false);
    }
    if (m_pMinLevelEdit) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%u",
                      static_cast<unsigned>(kDefaultMinLevel));
        m_pMinLevelEdit->SetEditText(buf);
    }
    if (m_pMaxLevelEdit) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%u",
                      static_cast<unsigned>(kDefaultMaxLevel));
        m_pMaxLevelEdit->SetEditText(buf);
    }
}

void cPartyCreateDlg::InitOption() {
    ApplyOptionDefaults();
}

void cPartyCreateDlg::Linking() {
    m_pThemeEdit = dynamic_cast<cEditBox*>(findWindowById(kThemeEditId));
    m_pMinLevelEdit = dynamic_cast<cEditBox*>(findWindowById(kMinLevelEditId));
    m_pMaxLevelEdit = dynamic_cast<cEditBox*>(findWindowById(kMaxLevelEditId));
    m_pPublicCheck = dynamic_cast<cCheckBox*>(findWindowById(kPublicCheckId));
    m_pPrivateCheck = dynamic_cast<cCheckBox*>(findWindowById(kPrivateCheckId));
    m_pDistribute = dynamic_cast<cComboBox*>(findWindowById(kDistributeComboId));
    m_pMemberNumCombo = dynamic_cast<cComboBox*>(findWindowById(kMemberNumComboId));
    m_pOKBtn = dynamic_cast<cButton*>(findWindowById(kOkButtonId));
    m_pCancelBtn = dynamic_cast<cButton*>(findWindowById(kCancelButtonId));
    m_bProcessing = false;
    InitOption();
}

void cPartyCreateDlg::SetActive(bool val) noexcept {
    cDialog::SetActive(val);
    if (!val) {
        ApplyOptionDefaults();
    }
}

bool cPartyCreateDlg::OnActionEvent(std::int32_t lId, void* /*p*/,
                                     std::uint32_t we) {
    if ((we & kActionBtnClick) == 0) {
        return false;
    }
    switch (lId) {
    case kPublicCheckId:
        if (m_pPrivateCheck) {
            m_pPrivateCheck->SetChecked(false);
        }
        return true;
    case kPrivateCheckId:
        if (m_pPublicCheck) {
            m_pPublicCheck->SetChecked(false);
        }
        return true;
    case kOkButtonId:
        if (CreatePartySyn()) {
            SetActive(false);
        }
        return true;
    case kCancelButtonId:
        SetActive(false);
        return true;
    default:
        return false;
    }
}

bool cPartyCreateDlg::CreatePartySyn() {
    PartyCreateOptions opts{};
    if (m_pThemeEdit) {
        std::string themeStr = m_pThemeEdit->editText();
        const char* theme = themeStr.c_str();
        const std::size_t len = theme ? std::strlen(theme) : 0;
        if (len > static_cast<std::size_t>(kMaxPartyNameLength)) {
            DispatchChatMessage(kChatPartyNameTooLong);
            return false;
        }
        if (theme) {
            std::strncpy(opts.theme, theme, sizeof(opts.theme) - 1);
        }
    }
    if (m_pMinLevelEdit) {
        opts.minLevel = static_cast<std::uint16_t>(
            std::atoi(m_pMinLevelEdit->editText().c_str()));
    }
    if (m_pMaxLevelEdit) {
        opts.maxLevel = static_cast<std::uint16_t>(
            std::atoi(m_pMaxLevelEdit->editText().c_str()));
    }
    opts.isPublic = m_pPublicCheck ? m_pPublicCheck->IsChecked() : false;

    if (m_pDistribute) {
        char buf[kDistributeBufferSize]{};
        std::string distStr = m_pDistribute->GetComboText();
        const char* txt = distStr.c_str();
        if (txt) {
            std::strncpy(buf, txt, sizeof(buf) - 1);
        }
        opts.division = ResolveDivisionOption(buf);
    }

    if (m_pMemberNumCombo) {
        std::string memStr = m_pMemberNumCombo->GetComboText();
        const char* txt = memStr.c_str();
        opts.limitCount = static_cast<std::uint16_t>(
            txt ? std::atoi(txt) : 0);
    }

    if (m_hasPartyFn && m_hasPartyFn(m_callbackUserData)) {
        return false;
    }

    bool sent = true;
    if (m_createSynFn) {
        sent = m_createSynFn(opts, m_callbackUserData);
    }
    if (sent) {
        m_bProcessing = true;
    }
    return sent;
}

} // namespace mxh::ui
