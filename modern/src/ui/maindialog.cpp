// maindialog.cpp — 1:1 port of 墨香
// CMainDialog (main UI button bar dialog).
// See maindialog.hpp for the data-model rationale
// + 1:1 quirks.

#include "maindialog.hpp"
#include "cpushupbutton.hpp"

namespace mxh::ui {

cMainDialog::cMainDialog() {
    // 1:1 with legacy CMainDialog ctor:
    //   m_type = WT_MAINDIALOG;
    //   for(int i = 0; i < MAX_BTN; i++) m_pBtn[i] = NULL;
    //
    // 1:1 quirk: modern cWindow does not have
    // m_type field (removed in Phase 6 when cWindow
    // was modernized). The ctor body is dropped.
    // The m_pBtn[i] zero-init is the default
    // unique_ptr behavior (empty unique_ptr is
    // null).
}

cMainDialog::~cMainDialog() = default;

void cMainDialog::Linking() {
    // 1:1 with legacy CMainDialog::Linking (the
    // legacy uses Add() side-channel to capture
    // cPushupButton references; modern port
    // synthesizes the children directly).
    //
    // The legacy:
    //   m_pBtn[CHAR_BTN=0]      (no MI_CHARBTN in legacy
    //                            Add; legacy comment-only)
    //   m_pBtn[INVENTORY_BTN=1] if !m_pBtn[1] && id == MI_INVENTORYBTN
    //   m_pBtn[MUGONG_BTN=2]    if !m_pBtn[2] && id == MI_MUGONGBTN
    //   m_pBtn[PARTY_BTN=4]     if !m_pBtn[4] && id == MI_PARTYBTN
    //
    // The modern port synths 4 cPushupButton with
    // the 4 ids and stores them. The Add() override
    // is replaced by Linking() since modern
    // cWindow::Add is non-virtual.
    m_pBtn[kIdxInventory] = std::make_unique<cPushupButton>();
    m_pBtn[kIdxInventory]->setId(kIdInventoryBtn);
    m_pBtn[kIdxMugong] = std::make_unique<cPushupButton>();
    m_pBtn[kIdxMugong]->setId(kIdMugongBtn);
    m_pBtn[kIdxChar] = std::make_unique<cPushupButton>();
    m_pBtn[kIdxChar]->setId(kIdCharBtn);
    m_pBtn[kIdxParty] = std::make_unique<cPushupButton>();
    m_pBtn[kIdxParty]->setId(kIdPartyBtn);
}

cPushupButton* cMainDialog::GetPushupBtn(std::uint16_t idx) const noexcept {
    // 1:1 with legacy CMainDialog::GetPushupBtn.
    // The legacy is:
    //   cPushupButton * GetPushupBtn(WORD idx) {
    //     return m_pBtn[idx];
    //   }
    //
    // The modern port bounds-checks idx against
    // kNumBtns to avoid OOB (legacy is UB; modern
    // is defensive).
    if (idx < kNumBtns) {
        return m_pBtn[idx].get();
    }
    return nullptr;
}

}  // namespace mxh::ui
