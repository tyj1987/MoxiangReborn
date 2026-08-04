// mpmissiondialog.cpp — 1:1 port of 墨香
// CMPMissionDialog (event-map mission notice
// dialog). See mpmissiondialog.hpp for the
// data-model rationale + 1:1 quirks.

#include "mpmissiondialog.hpp"
#include "ctextarea.hpp"

namespace mxh::ui {

cMPMissionDialog::cMPMissionDialog() {
    // 1:1 with legacy CMPMissionDialog ctor:
    //   m_type = WT_MPMISSIONDLG;
    //   ZeroMemory(m_pMissionMsg, sizeof(m_pMissionMsg));
    //   ZeroMemory(m_pCautionMsg, sizeof(m_pCautionMsg));
    //
    // 1:1 quirk: modern cWindow does not have
    // m_type field (removed in Phase 6 when cWindow
    // was modernized). The ctor body is dropped.
    // The m_pMissionMsg / m_pCautionMsg zero-init
    // is the default std::vector behavior (empty
    // vector). LoadMissionMsg is COMMENTED OUT in
    // legacy ctor (and is a no-op in modern port).
}

cMPMissionDialog::~cMPMissionDialog() = default;

void cMPMissionDialog::SetCurrentTimeProvider(
    MpClockFn getCurrentTime, void* userData) noexcept {
    m_getCurrentTimeFn = getCurrentTime;
    m_clockUserData = userData;
}

void cMPMissionDialog::SetCallbacks(
    GetChatMessageFn     getChatMessage,
    ShowMPNoticeDialogFn showMPNoticeDialog,
    void*                userData) noexcept {
    m_getChatMessage     = getChatMessage;
    m_showMPNoticeDialog = showMPNoticeDialog;
    m_callbackUserData   = userData;
}

void cMPMissionDialog::Linking() {
    // 1:1 with legacy CMPMissionDialog::Linking.
    // The legacy is:
    //   m_pMission = (cTextArea*)GetWindowForID(MP_MMISSION);
    //   m_pCaution = (cTextArea*)GetWindowForID(MP_MCAUTION);
    //   m_pMission->SetScriptText(CHATMGR->GetChatMsg(665));
    //   m_pCaution->SetScriptText(CHATMGR->GetChatMsg(666));
    //   m_dwStartTime = 0;
    m_pMission =
        static_cast<cTextArea*>(findWindowById(kIdMission));
    m_pCaution =
        static_cast<cTextArea*>(findWindowById(kIdCaution));
    if (m_pMission) {
        const char* msg = m_getChatMessage
            ? m_getChatMessage(kChatMsgMission, m_callbackUserData)
            : nullptr;
        // 1:1 with legacy CHATMGR->GetChatMsg(665)
        // -- resolved via host callback, with
        // kMissionText as the placeholder
        // fallback when the callback is absent.
        m_pMission->SetScriptText(msg ? msg : kMissionText);
    }
    if (m_pCaution) {
        const char* msg = m_getChatMessage
            ? m_getChatMessage(kChatMsgCaution, m_callbackUserData)
            : nullptr;
        m_pCaution->SetScriptText(msg ? msg : kCautionText);
    }
    m_dwStartTime = 0;
}

void cMPMissionDialog::SetMissionInfo(int msgnum) {
    // 1:1 with legacy CMPMissionDialog::SetMissionInfo.
    // The legacy is:
    //   if (msgnum >= MAX_MISSIONMSG_NUM) {
    //     ASSERT(0);
    //     msgnum = 0;
    //   }
    //   m_pMission->SetScriptText(m_pMissionMsg[msgnum]);
    //   m_pCaution->SetScriptText(m_pCautionMsg[msgnum]);
    //
    // The modern port:
    //   - Uses defensive bounds-check instead of
    //     ASSERT (modern tests can't trigger asserts
    //     in production code).
    //   - m_pMissionMsg / m_pCautionMsg are
    //     std::vector<std::string> in modern port;
    //     since LoadMissionMsg is a no-op, the
    //     vectors are empty, so SetMissionInfo is
    //     effectively a no-op in modern port.
    if (msgnum < 0 || msgnum >= kMaxMissionMsgNum) {
        // 1:1 with legacy ASSERT(0); modern
        // silently returns.
        return;
    }
    if (msgnum >= static_cast<int>(m_pMissionMsg.size())) {
        // Vector is empty (LoadMissionMsg no-op);
        // nothing to do.
        return;
    }
    if (m_pMission) {
        m_pMission->SetScriptText(m_pMissionMsg[msgnum].c_str());
    }
    if (m_pCaution) {
        m_pCaution->SetScriptText(m_pCautionMsg[msgnum].c_str());
    }
}

void cMPMissionDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CMPMissionDialog::SetActive
    // override. The legacy is:
    //   if (val == FALSE) {
    //     GAMEIN->GetMPNoticeDialog()->SetActive(TRUE);
    //   } else {
    //     m_dwStartTime = gCurTime;
    //   }
    //   cDialog::SetActive(val);
    //
    // The modern port:
    //   - val==TRUE gCurTime stamp is REAL via
    //     OPTIONAL host clock provider (zero-clock
    //     fallback when null).
    //   - val==FALSE GAMEIN->GetMPNoticeDialog dispatch
    //     is TODO (R-12.x deferred).
    //   - Always calls base SetActive(val) (matches
    //     legacy call order).
    if (val) {
        m_dwStartTime = m_getCurrentTimeFn
            ? m_getCurrentTimeFn(m_clockUserData)
            : 0u;
    } else {
        // 1:1 with legacy:
        //   GAMEIN->GetMPNoticeDialog()->SetActive(TRUE);
        // Dispatched BEFORE the base
        // SetActive(val) so the MPNoticeDialog
        // activates before the MPMissionDialog
        // closes (matches legacy call order
        // byte-for-byte).
        if (m_showMPNoticeDialog) {
            m_showMPNoticeDialog(m_callbackUserData);
        }
    }
    cDialog::SetActive(val);
}

std::uint32_t cMPMissionDialog::ActionEvent() {
    // 1:1 with legacy CMPMissionDialog::ActionEvent.
    // The legacy is:
    //   DWORD we = cDialog::ActionEvent(mouseInfo);
    //   if (IsActive()) {
    //     if (gCurTime - m_dwStartTime >= 5000) {
    //       SetActive(FALSE);
    //     }
    //   }
    //   return we;
    //
    // The modern port: the 5 sec auto-close gate is
    // REAL via the OPTIONAL host clock provider.
    // CMouse-based cDialog::ActionEvent dispatch is
    // TODO (CMouse not ported, R-12.x deferred).
    // Modern port returns WE_NULL matching the
    // legacy no-event path (DWORD wrap-around
    // preserved).
    if (isActive() && m_getCurrentTimeFn) {
        const std::uint32_t curTime = m_getCurrentTimeFn(m_clockUserData);
        if (curTime - m_dwStartTime >= 5000u) {
            SetActive(false);
        }
    }
    return 0;  // WE_NULL
}

}  // namespace mxh::ui
