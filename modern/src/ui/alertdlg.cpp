// alertdlg.cpp — 1:1 port of 墨香
// CAlertDialog (alert dialog). See alertdlg.hpp for
// the data-model rationale + 1:1 quirks.

#include "alertdlg.hpp"
#include "cbutton.hpp"
#include "cwindow.hpp"

namespace mxh::ui {

cAlertDlg::cAlertDlg() {
    // 1:1 with legacy CAlertDlg ctor:
    //   m_pOk=NULL; m_pCancel=NULL;
    //
    // 1:1 quirk: modern unique_ptr is default-null
    // (no explicit init needed). m_obj is default-null
    // (void* with = nullptr). m_cbBtnFunc is
    // default-empty (std::function).
}

cAlertDlg::~cAlertDlg() = default;

void cAlertDlg::Linking() {
    // 1:1 with legacy CAlertDlg::Linking (legacy
    // doesn't have an explicit Linking; the legacy
    // uses Add() side-channel to capture m_pOk /
    // m_pCancel from the resource loader).
    //
    // The modern port synthesizes 2 cButton (OK +
    // Cancel), stores non-owning raw pointers in
    // m_pOk / m_pCancel, and adds them to the
    // dialog children (so findWindowById works
    // in tests). The dialog owns the buttons via
    // its cWindow children list; m_pOk / m_pCancel
    // are non-owning accessors (1:1 with legacy
    // cButton* m_pOk pattern).
    auto okBtn = std::make_unique<cButton>();
    okBtn->Init(0, 0, 50, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
                kIdOkBtn);
    m_pOk = okBtn.get();
    Add(std::move(okBtn));

    auto cancelBtn = std::make_unique<cButton>();
    cancelBtn->Init(0, 30, 50, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
                    kIdCancelBtn);
    m_pCancel = cancelBtn.get();
    Add(std::move(cancelBtn));
}

std::uint32_t cAlertDlg::ActionEvent() {
    // 1:1 with legacy CAlertDlg::ActionEvent.
    // The legacy is:
    //   DWORD we = WE_NULL;
    //   DWORD we2 = WE_NULL;
    //   we |= cWindow::ActionEvent(mouseInfo);
    //   we |= cDialog::ActionEventWindow(mouseInfo);
    //   we2 = m_pOk->ActionEvent(mouseInfo);
    //   if (we2 & WE_BTNCLICK)
    //     cbBtnFunc(m_ID, this, 1);
    //   we2 = m_pCancel->ActionEvent(mouseInfo);
    //   if (we2 & WE_BTNCLICK)
    //     cbBtnFunc(m_ID, this, 0);
    //   return we;
    //
    // The modern port: the whole method is TODO
    // (CMouse not ported, R-12.x deferred). Modern
    // port returns WE_NULL. The cbBtnFunc dispatch
    // is preserved for the future CMouse port.
    // TODO: CMouse not ported (R-12.x deferred).
    //       When ported, the body becomes the legacy
    //       code with the cbBtnFunc dispatch.
    return 0;  // WE_NULL
}

void cAlertDlg::SetcbBtn(BtnCallback cbFunc) noexcept {
    // 1:1 with legacy SetcbBtn(void (*cbFunc)). The
    // modern port uses std::function (1:1 with
    // legacy C function pointer).
    m_cbBtnFunc = std::move(cbFunc);
}

}  // namespace mxh::ui
