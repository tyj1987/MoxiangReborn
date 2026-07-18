// progressbardlg.cpp — 1:1 port of 墨香
// CProgressBarDlg (base progress bar dialog).
// See progressbardlg.hpp for the data-model
// rationale + 1:1 quirks.

#include "progressbardlg.hpp"
#include "cobjectguagen.hpp"
#include "cstatic.hpp"

namespace mxh::ui {

cProgressBarDlg::cProgressBarDlg() {
    // 1:1 with legacy CProgressBarDlg ctor:
    //   m_pProgressGuagen = NULL;
    //   m_pRemaintimeStatic = NULL;
    //   m_bProgressStart = FALSE;
    //   m_bSuccessProgress = FALSE;
    //   m_dwProcessTime = 0;
    //   m_dwCurrentTime = 0;
    //   m_dwSuccessTime = 0;
    //
    // 1:1 quirk: modern raw pointers / bool /
    // std::uint32_t use default member init (= nullptr /
    // false / 0 in header). ctor body is empty.
}

cProgressBarDlg::~cProgressBarDlg() = default;

void cProgressBarDlg::SetActive(bool val) noexcept {
    // 1:1 with legacy CProgressBarDlg::SetActive.
    // The legacy is:
    //   cDialog::SetActive(val);
    //   if (val == FALSE) {
    //     m_bProgressStart = FALSE;
    //     m_dwProcessTime = 0;
    //     m_dwCurrentTime = 0;
    //     m_pProgressGuagen->SetValue(0, 0);
    //   }
    //
    // The modern port: SetActive is REAL (1:1 with
    // legacy). SetValue(0, 0) is REAL on
    // CObjectGuagen (just ported in 0.13.50).
    cDialog::SetActive(val);
    if (!val) {
        m_bProgressStart = false;
        m_dwProcessTime = 0;
        m_dwCurrentTime = 0;
        if (m_pProgressGuagen) {
            m_pProgressGuagen->SetValue(0.0f, 0);
        }
    }
}

void cProgressBarDlg::Process() {
    // 1:1 with legacy CProgressBarDlg::Process. The
    // legacy is:
    //   if (!m_bProgressStart) return;
    //   m_dwCurrentTime = gCurTime;
    //   float fGageValue = 1.0f - ((m_dwProcessTime -
    //                               m_dwCurrentTime) / m_dwSuccessTime);
    //   m_pProgressGuagen->SetValue(fGageValue, m_dwCurrentTime);
    //   if (m_dwProcessTime < m_dwCurrentTime) m_bSuccessProgress = TRUE;
    //   char buf[128];
    //   sprintf(buf, CHATMGR->GetChatMsg(1043),
    //           (m_dwProcessTime - m_dwCurrentTime + 1000) / 1000);
    //   m_pRemaintimeStatic->SetStaticText(buf);
    //
    // The modern port: the gCurTime + CHATMGR + tick
    // interpolation is TODO (R-12.x deferred). Modern
    // port returns without updating state.
    // TODO: gCurTime + CHATMGR not ported (R-12.x
    //       deferred). When ported, the body becomes
    //       the legacy code.
    if (!m_bProgressStart) return;
}

void cProgressBarDlg::StartProgress() {
    // 1:1 with legacy CProgressBarDlg::StartProgress.
    // The legacy is:
    //   InitProgress();
    //   m_bProgressStart = TRUE;
    //   m_dwCurrentTime = gCurTime;
    //   m_dwProcessTime = m_dwCurrentTime + m_dwSuccessTime;
    //   SetActive(TRUE);
    //
    // The modern port: InitProgress + m_bProgressStart
    // + SetActive(TRUE) are REAL. The gCurTime-based
    // m_dwCurrentTime / m_dwProcessTime updates are
    // TODO.
    InitProgress();
    m_bProgressStart = true;
    // TODO: m_dwCurrentTime = gCurTime;
    //       m_dwProcessTime = m_dwCurrentTime + m_dwSuccessTime;
    SetActive(true);
}

void cProgressBarDlg::InitProgress() {
    // 1:1 with legacy CProgressBarDlg::InitProgress.
    // The legacy is:
    //   m_bProgressStart = FALSE;
    //   m_bSuccessProgress = FALSE;
    //   m_dwProcessTime = 0;
    //   m_dwCurrentTime = 0;
    //   SetActive(FALSE);
    m_bProgressStart = false;
    m_bSuccessProgress = false;
    m_dwProcessTime = 0;
    m_dwCurrentTime = 0;
    SetActive(false);
}

void cProgressBarDlg::Render() {
    // 1:1 with legacy CProgressBarDlg::Render. The
    // legacy is:
    //   Process();
    //   cDialog::Render();
    //
    // The modern port: Process is TODO + cDialog::Render
    // is no-op. Modern port is a no-op.
    // TODO: Process() call (R-12.x deferred).
}

}  // namespace mxh::ui
