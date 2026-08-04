// tipbrowserdlg.cpp — 1:1 port of 墨香 CTipBrowserDlg
// (tip browser dialog). See tipbrowserdlg.hpp for
// the data-model rationale + 1:1 quirks.

#include "tipbrowserdlg.hpp"
#include "cpushupbutton.hpp"
#include "legacy_window_event.hpp"

namespace mxh::ui {

cTipBrowserDlg::cTipBrowserDlg() {
    // 1:1 with legacy CTipBrowserDlg::CTipBrowserDlg:
    //   for (int i = 0; i < 4; i++) {
    //     m_pButton[i] = NULL;
    //     m_pDlg[i] = NULL;
    //   }
    //   m_wCurDlg = 0;
    // Modern port uses default member init in the
    // header (m_pButton[i] = nullptr, m_pDlg[i] =
    // nullptr, m_wCurDlg = 0).
}

cTipBrowserDlg::~cTipBrowserDlg() = default;

void cTipBrowserDlg::Linking() {
    // 1:1 with legacy CTipBrowserDlg::Linking. The
    // legacy is:
    //   for (int i = 0; i < 4; i++) {
    //     m_pDlg[i] = (cDialog*)GetWindowForID(TB_STATE_PO + i);
    //     m_pButton[i] = (cPushupButton*)GetWindowForID(TB_STATE_PUSHUP1 + i);
    //   }
    //   m_wCurDlg = 0;
    for (std::size_t i = 0; i < kNumTabs; ++i) {
        m_pDlg[i] = static_cast<cDialog*>(findWindowById(kIdStateBase + static_cast<std::int32_t>(i)));
        m_pButton[i] = static_cast<cPushupButton*>(findWindowById(kIdPushupBase + static_cast<std::int32_t>(i)));
    }
    m_wCurDlg = 0;
}

void cTipBrowserDlg::Show() {
    // 1:1 with legacy CTipBrowserDlg::Show. The
    // legacy is:
    //   SetActive(TRUE);
    //   for (int i = 0; i < 4; i++) {
    //     m_pDlg[i]->SetActive(FALSE);
    //     m_pButton[i]->SetPush(FALSE);
    //     m_pButton[i]->SetDisable(FALSE);
    //   }
    //   m_pDlg[m_wCurDlg]->SetActive(TRUE);
    //   m_pButton[m_wCurDlg]->SetPush(TRUE);
    //   m_pButton[m_wCurDlg]->SetDisable(TRUE);
    // }  // 1:1 quirk: closing brace placement
    //     // is a bit unusual in the legacy;
    //     // modern port follows the corrected
    //     // control flow.
    SetActive(true);
    for (std::size_t i = 0; i < kNumTabs; ++i) {
        if (m_pDlg[i])     m_pDlg[i]->SetActive(false);
        if (m_pButton[i])  m_pButton[i]->SetPush(false);
        if (m_pButton[i])  m_pButton[i]->SetDisable(false);
    }
    if (m_wCurDlg < kNumTabs) {
        if (m_pDlg[m_wCurDlg])    m_pDlg[m_wCurDlg]->SetActive(true);
        if (m_pButton[m_wCurDlg]) m_pButton[m_wCurDlg]->SetPush(true);
        if (m_pButton[m_wCurDlg]) m_pButton[m_wCurDlg]->SetDisable(true);
    }
}

void cTipBrowserDlg::Close() {
    // 1:1 with legacy CTipBrowserDlg::Close. The
    // legacy is:
    //   SetActive(FALSE);
    //   m_wCurDlg = 0;
    SetActive(false);
    m_wCurDlg = 0;
}

void cTipBrowserDlg::OnActionEvent(std::int32_t lId, void* p, std::uint32_t we) {
    // 1:1 with legacy CTipBrowserDlg::OnActionEvent.
    // The legacy is:
    //   if (we == WE_PUSHDOWN) {  // 1:1 quirk: exact
    //                              // match, not bit-and
    //     WORD id = (WORD)(lId - TB_STATE_PUSHUP1);
    //     if (id < 4 && (lId - TB_STATE_PUSHUP1) >= 0) {
    //       m_wCurDlg = id;
    //       Show();
    //       return;
    //     }
    //   }
    //   if (we & WE_BTNCLICK && lId == TB_CANCELBTN) {
    //     Close();
    //   }
    //
    // The modern port:
    //   - Uses kIdPushupBase (= TB_STATE_PUSHUP1)
    //     and kIdCancelBtn (= TB_CANCELBTN).
    //   - WE_PUSHDOWN uses `==` exact match (1:1
    //     quirk preserved; not bit-and).
    //   - WE_BTNCLICK uses `&` bit-and (1:1 with
    //     legacy).
    (void)p;
    constexpr std::uint32_t WE_BTNCLICK = legacy_window_event::kButtonClick;
    constexpr std::uint32_t WE_PUSHDOWN  = legacy_window_event::kPushDown;  // legacy cWindow::we
    if (we == WE_PUSHDOWN) {
        // 1:1 quirk: legacy uses `==` not `&` for
        // WE_PUSHDOWN (exact match required).
        std::int32_t offset = lId - kIdPushupBase;
        if (offset >= 0 && offset < static_cast<std::int32_t>(kNumTabs)) {
            m_wCurDlg = static_cast<std::uint16_t>(offset);
            Show();
            return;
        }
    }
    if ((we & WE_BTNCLICK) && lId == kIdCancelBtn) {
        Close();
    }
}

}  // namespace mxh::ui
