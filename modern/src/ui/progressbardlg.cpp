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

void cProgressBarDlg::SetCurrentTimeProvider(
    PbClockFn getCurrentTime, void* userData) noexcept {
    m_getCurrentTimeFn = getCurrentTime;
    m_clockUserData = userData;
}

void cProgressBarDlg::SetChatMessageFn(
    PbChatMsgFn getChatMsg, void* userData) noexcept {
    m_getChatMsgFn = getChatMsg;
    m_chatUserData = userData;
}

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
    //   sprintf(buf, CHATMGR->GetChatMsg(1043),
    //           (m_dwProcessTime - m_dwCurrentTime + 1000) / 1000);
    //   m_pRemaintimeStatic->SetStaticText(buf);
    //
    // The modern port: gCurTime and CHATMGR are routed
    // through OPTIONAL host callbacks. A null clock
    // provider preserves the safe zero-clock fallback
    // (m_dwCurrentTime = 0, m_dwProcessTime unchanged).
    // A null chat fn falls back to a literal
    // placeholder so tests can verify both branches.
    if (!m_bProgressStart) return;
    const std::uint32_t curTime = m_getCurrentTimeFn
        ? m_getCurrentTimeFn(m_clockUserData)
        : 0u;
    m_dwCurrentTime = curTime;
    if (m_pProgressGuagen && m_dwSuccessTime > 0u) {
        const float fGageValue = 1.0f
            - static_cast<float>(m_dwProcessTime - m_dwCurrentTime)
              / static_cast<float>(m_dwSuccessTime);
        m_pProgressGuagen->SetValue(fGageValue, m_dwCurrentTime);
    }
    if (m_dwProcessTime < m_dwCurrentTime) {
        m_bSuccessProgress = true;
    }
    if (m_pRemaintimeStatic) {
        constexpr std::size_t kFmtBufSize = 128;
        char buf[kFmtBufSize] = {};
        const char* fmt = m_getChatMsgFn
            ? m_getChatMsgFn(1043, m_chatUserData)
            : "Remaining %d sec";
        std::snprintf(buf, sizeof(buf), fmt,
            static_cast<int>(
                (m_dwProcessTime - m_dwCurrentTime + 1000u) / 1000u));
        m_pRemaintimeStatic->SetStaticText(buf);
    }
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
    // The modern port: gCurTime is routed through the
    // OPTIONAL host clock provider. A null provider
    // preserves the safe zero-clock fallback.
    InitProgress();
    m_bProgressStart = true;
    const std::uint32_t curTime = m_getCurrentTimeFn
        ? m_getCurrentTimeFn(m_clockUserData)
        : 0u;
    m_dwCurrentTime = curTime;
    m_dwProcessTime = curTime + m_dwSuccessTime;
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
    // The modern port: Process() is REAL (now wired
    // with OPTIONAL clock + chat callbacks);
    // cDialog::Render is still no-op in the test
    // environment.
    Process();
    cDialog::Render();
}

}  // namespace mxh::ui
