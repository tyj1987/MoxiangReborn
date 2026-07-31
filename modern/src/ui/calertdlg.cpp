// calertdlg.cpp -- modern implementation of Moxiang
//   CAlertDlg (alert dialog: OK/Cancel buttons + callback).

#include "calertdlg.hpp"

#include "cbutton.hpp"
#include "cwindow.hpp"

namespace mxh::ui {

cAlertDlg::cAlertDlg() = default;

cAlertDlg::~cAlertDlg() = default;

void cAlertDlg::Linking() {
    // 1:1 with legacy CAlertDlg::Linking -- but the
    // legacy has no explicit Linking; it uses the Add()
    // side-channel to capture m_pOk / m_pCancel from the
    // resource loader.
    //
    // Modern port resolves the 2 cButton via
    // findWindowById (same pattern as cStallKindSelectDlg
    // and other 1:1 ports). Host-injected pointers
    // (SetOkBtnForTest / SetCancelBtnForTest) take
    // priority over auto-discovery.
    if (!m_pOk) {
        m_pOk = static_cast<cButton*>(findWindowById(kIdOkBtn));
    }
    if (!m_pCancel) {
        m_pCancel = static_cast<cButton*>(findWindowById(kIdCancelBtn));
    }
}

void cAlertDlg::OnActionEvent(std::int32_t lId, void* /*p*/, std::uint32_t we) {
    // 1:1 with legacy CAlertDlg::ActionEvent(CMouse*) --
    // the WE_BTNCLICK dispatch portion. The legacy body is:
    //   DWORD we = WE_NULL;
    //   we |= cWindow::ActionEvent(mouseInfo);
    //   we |= cDialog::ActionEventWindow(mouseInfo);
    //   we2 = m_pOk->ActionEvent(mouseInfo);
    //   if (we2 & WE_BTNCLICK) cbBtnFunc(m_ID, this, 1);
    //   we2 = m_pCancel->ActionEvent(mouseInfo);
    //   if (we2 & WE_BTNCLICK) cbBtnFunc(m_ID, this, 0);
    //   return we;
    //
    // Modern port: the cWindow::ActionEvent recursion
    // handles the m_pOk / m_pCancel state machines (each
    // cButton's ActionEvent is its own state machine that
    // fires m_onClick on a successful click). The
    // OnActionEvent hook here is the public dispatch
    // target for the button's m_onClick (wired at link
    // time by tests / integration). This matches the
    // cStallKindSelectDlg pattern (1:1 with legacy
    // OnActionEvent -> OnActionEvnet typo).
    //
    // 1:1 with legacy's `if (we2 & WE_BTNCLICK)`.
    if (!(we & kWeBtnClick)) {
        return;
    }

    // 1:1 with legacy's `m_ID` (the dialog's id, passed
    // as the first arg to cbBtnFunc). In modern, this is
    // cObject::m_id.
    m_lastLId = lId;
    m_lastWe  = we;

    if (lId == kIdOkBtn) {
        // 1:1 with legacy OK branch: cbBtnFunc(m_ID, this, 1).
        if (m_cbBtnFunc) {
            m_cbBtnFunc(id(), this, 1);
        }
        ++m_okDispatchCount;
    } else if (lId == kIdCancelBtn) {
        // 1:1 with legacy Cancel branch: cbBtnFunc(m_ID, this, 0).
        if (m_cbBtnFunc) {
            m_cbBtnFunc(id(), this, 0);
        }
        ++m_cancelDispatchCount;
    }
}

void cAlertDlg::SetcbBtn(BtnCallback cbFunc) noexcept {
    // 1:1 with legacy SetcbBtn(void (*cbFunc)). The
    // modern port uses std::function (1:1 with legacy C
    // function pointer semantics).
    m_cbBtnFunc = std::move(cbFunc);
}

}  // namespace mxh::ui