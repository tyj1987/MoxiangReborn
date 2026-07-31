#include "partybtndlg.hpp"

#include "mxh/ui/cButton.hpp"
#include "mxh/ui/cStatic.hpp"

namespace mxh::ui {

cPartyBtnDlg::cPartyBtnDlg() = default;

cPartyBtnDlg::~cPartyBtnDlg() = default;

void cPartyBtnDlg::SetControlsForTest(cStatic* background, cButton* secede,
                                               cButton* transfer, cButton* forcedSecede,
                                               cButton* addMember, cButton* breakUp,
                                               cButton* warSuggest, cButton* option,
                                               cButton* member) noexcept {
    m_pBackGround = background;
    m_pSecedeBtn = secede;
    m_pTransferBtn = transfer;
    m_pForcedSecedeBtn = forcedSecede;
    m_pAddMemberBtn = addMember;
    m_pBreakUpBtn = breakUp;
    m_pWarSuggestBtn = warSuggest;
    m_pOptionBtn = option;
    m_pMemberBtn = member;
}

void cPartyBtnDlg::Linking() {
    m_pBackGround = dynamic_cast<cStatic*>(findWindowById(kBackgroundId));
    m_pSecedeBtn = dynamic_cast<cButton*>(findWindowById(kSecedeButtonId));
    m_pTransferBtn = dynamic_cast<cButton*>(findWindowById(kTransferButtonId));
    m_pForcedSecedeBtn = dynamic_cast<cButton*>(findWindowById(kForcedSecedeButtonId));
    m_pAddMemberBtn = dynamic_cast<cButton*>(findWindowById(kAddMemberButtonId));
    m_pBreakUpBtn = dynamic_cast<cButton*>(findWindowById(kBreakUpButtonId));
    m_pWarSuggestBtn = dynamic_cast<cButton*>(findWindowById(kWarSuggestButtonId));
    m_pOptionBtn = dynamic_cast<cButton*>(findWindowById(kOptionButtonId));
    m_pMemberBtn = dynamic_cast<cButton*>(findWindowById(kMemberButtonId));
}

void cPartyBtnDlg::RefreshDlg() {
    if (m_partyState.partyIndex == 0) {
        ShowNonPartyDlg();
    } else if (m_partyState.masterId == m_partyState.heroId) {
        ShowPartyMasterDlg();
    } else {
        ShowPartyMemberDlg();
    }
}

void cPartyBtnDlg::SetActionButtonsActive(bool active) {
    if (m_pSecedeBtn != nullptr) m_pSecedeBtn->SetActive(active);
    if (m_pTransferBtn != nullptr) m_pTransferBtn->SetActive(active);
    if (m_pForcedSecedeBtn != nullptr) m_pForcedSecedeBtn->SetActive(active);
    if (m_pAddMemberBtn != nullptr) m_pAddMemberBtn->SetActive(active);
    if (m_pBreakUpBtn != nullptr) m_pBreakUpBtn->SetActive(active);
    if (m_pWarSuggestBtn != nullptr) m_pWarSuggestBtn->SetActive(active);
}

void cPartyBtnDlg::SetActionImageColors(std::uint32_t secedeColor,
                                           std::uint32_t otherColor) {
    if (m_pSecedeBtn != nullptr) m_pSecedeBtn->SetImageRGB(secedeColor);
    if (m_pTransferBtn != nullptr) m_pTransferBtn->SetImageRGB(otherColor);
    if (m_pForcedSecedeBtn != nullptr) m_pForcedSecedeBtn->SetImageRGB(otherColor);
    if (m_pAddMemberBtn != nullptr) m_pAddMemberBtn->SetImageRGB(otherColor);
    if (m_pBreakUpBtn != nullptr) m_pBreakUpBtn->SetImageRGB(otherColor);
    if (m_pWarSuggestBtn != nullptr) m_pWarSuggestBtn->SetImageRGB(otherColor);
}

void cPartyBtnDlg::ShowNonPartyDlg() {
    SetActionButtonsActive(false);
    if (m_pOptionBtn != nullptr) m_pOptionBtn->SetActive(false);
    if (m_pMemberBtn != nullptr) m_pMemberBtn->SetActive(false);
}

void cPartyBtnDlg::ShowPartyMasterDlg() {
    if (m_pBackGround != nullptr) m_pBackGround->SetActive(m_bOption);
    SetActionButtonsActive(m_bOption);
    if (m_pOptionBtn != nullptr) m_pOptionBtn->SetActive(true);
    if (m_pMemberBtn != nullptr) m_pMemberBtn->SetActive(true);
    SetActionImageColors(kDisabledImageColor, kUsableImageColor);
}

void cPartyBtnDlg::ShowPartyMemberDlg() {
    if (m_pBackGround != nullptr) m_pBackGround->SetActive(m_bOption);
    SetActionButtonsActive(m_bOption);
    if (m_pOptionBtn != nullptr) m_pOptionBtn->SetActive(true);
    if (m_pMemberBtn != nullptr) m_pMemberBtn->SetActive(true);
    SetActionImageColors(kUsableImageColor, kDisabledImageColor);
}

void cPartyBtnDlg::ShowOption(bool option) {
    m_bOption = option;
    if (m_pBackGround != nullptr) m_pBackGround->SetActive(m_bOption);
    SetActionButtonsActive(m_bOption);
    if (m_pOptionBtn != nullptr) m_pOptionBtn->SetActive(true);
    if (m_pMemberBtn != nullptr) m_pMemberBtn->SetActive(true);
}

void cPartyBtnDlg::Render() {
    if (m_pMemberBtn != nullptr && m_pOptionBtn != nullptr) {
        const std::int32_t offset = m_bOption ? 100 : 20;
        m_pMemberBtn->SetAbsXY(m_pMemberBtn->absX(),
                                m_pOptionBtn->absY() + offset);
    }
    cDialog::Render();
}

}  // namespace mxh::ui
