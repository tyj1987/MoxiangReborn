// wantregistdialog.cpp — 1:1 port of 墨香
// CWantRegistDialog (wanted registration editor
// dialog). See wantregistdialog.hpp for the
// data-model rationale + 1:1 quirks.

#include "wantregistdialog.hpp"
#include "cstatic.hpp"
#include "ceditbox.hpp"

namespace mxh::ui {

cWantRegistDialog::cWantRegistDialog() {
    // 1:1 with legacy CWantRegistDialog ctor:
    //   m_type = WT_WANTREGISTDIALOG;
    //
    // 1:1 quirk: modern cWindow does not have
    // m_type field (removed in Phase 6 when cWindow
    // was modernized). The ctor body is dropped.
    // m_bShow and m_dwStartShowTime use their legacy false/zero
    // in-class initial values.
}

cWantRegistDialog::~cWantRegistDialog() = default;

void cWantRegistDialog::SetCurrentTimeProvider(
    WgClockFn getCurrentTime, void* userData) noexcept {
    m_getCurrentTimeFn = getCurrentTime;
    m_clockUserData = userData;
}

void cWantRegistDialog::SetCancelCallbacks(
    GetHeroObjectIdFn getHeroObjectId,
    SendRegistCancelFn sendRegistCancel,
    void* userData) noexcept {
    m_getHeroObjectIdFn = getHeroObjectId;
    m_sendRegistCancelFn = sendRegistCancel;
    m_cancelUserData = userData;
}

void cWantRegistDialog::Linking() {
    // 1:1 with legacy CWantRegistDialog::Linking.
    // The legacy is:
    //   m_WantedName = (cStatic*)GetWindowForID(WANTREG_WANTEDNAME);
    //   m_PrizeEdit = (cEditBox*)GetWindowForID(WANTREG_PRIZEEDIT);
    //   m_PrizeEdit->SetValidCheck(VCM_NUMBER);
    //   m_bShow = FALSE;
    //   m_dwStartShowTime = 0;
    m_WantedName = static_cast<cStatic*>(findWindowById(kIdWantedName));
    m_PrizeEdit  = static_cast<cEditBox*>(findWindowById(kIdPrizeEdit));
    if (m_PrizeEdit) {
        m_PrizeEdit->SetValidCheck(kVcmNumber);
    }
    m_bShow = false;
    m_dwStartShowTime = 0;
}

void cWantRegistDialog::SetWantedName(const char* pName) {
    // 1:1 with legacy CWantRegistDialog::SetWantedName.
    // The legacy is:
    //   m_WantedName->SetStaticText(pName);
    //   m_PrizeEdit->SetEditText("");
    //
    // The modern port:
    //   - Uses std::string for pName (1:1 with
    //     legacy char* c-string, since the
    //     SetStaticText + SetEditText calls are
    //     just simple setters).
    //   - 1:1 quirk: modern port guards null pName
    //     (legacy would crash on null).
    if (m_WantedName && pName) {
        m_WantedName->SetStaticText(pName);
    }
    if (m_PrizeEdit) {
        m_PrizeEdit->SetEditText("");
    }
}

void cWantRegistDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CWantRegistDialog::SetActive
    // override. The legacy is:
    //   if (val == m_bActive) return;
    //   if (val == TRUE) {
    //     m_dwStartShowTime = gCurTime;
    //   } else {
    //     m_PrizeEdit->SetFocusEdit(FALSE);
    //     MSGBASE msg;
    //     msg.Category = MP_WANTED;
    //     msg.Protocol = MP_WANTED_REGIST_CANCEL;
    //     msg.dwObjectID = HEROID;
    //     NETWORK->Send(&msg, sizeof(msg));
    //   }
    //   m_bShow = FALSE;
    //   cDialog::SetActive(val);
    //
    // Preserve the legacy same-active early return before every
    // side effect, including clock reads and cancel sends.
    if (val == isActive()) {
        return;
    }
    if (val) {
        // 1:1 with legacy: m_dwStartShowTime = gCurTime.
        // m_bShow gates ActionEvent so the dialog stays
        // invisible until 3 sec have passed.
        m_dwStartShowTime = m_getCurrentTimeFn
            ? m_getCurrentTimeFn(m_clockUserData)
            : 0u;
    } else {
        if (m_PrizeEdit) {
            m_PrizeEdit->SetFocusEdit(false);
        }
        if (m_getHeroObjectIdFn && m_sendRegistCancelFn) {
            const std::uint32_t objectId =
                m_getHeroObjectIdFn(m_cancelUserData);
            (void)m_sendRegistCancelFn(objectId, m_cancelUserData);
        }
    }
    m_bShow = false;
    cDialog::SetActive(val);
}

std::uint32_t cWantRegistDialog::ActionEvent() {
    // 1:1 with legacy CWantRegistDialog::ActionEvent.
    // The legacy is:
    //   DWORD we = WE_NULL;
    //   if (m_bDisable || !m_bActive) return we;
    //   if (!m_bShow) {
    //     if (gCurTime - m_dwStartShowTime >= 3000) {
    //       m_bShow = TRUE;
    //     } else {
    //       return we;
    //     }
    //   }
    //   we = cDialog::ActionEvent(mouseInfo);
    //   return we;
    //
    // The modern port: the gCurTime-based
    // m_bShow / m_dwStartShowTime gating is REAL via
    // OPTIONAL host clock provider. The CMouse-based
    // cDialog::ActionEvent dispatch is TODO (CMouse
    // not ported, R-12.x deferred). Modern port
    // returns WE_NULL matching the legacy early
    // return path (DWORD wrap-around preserved).
    if (!isEnabled() || !isActive()) {
        return 0;
    }
    if (m_getCurrentTimeFn && !m_bShow) {
        const std::uint32_t curTime = m_getCurrentTimeFn(m_clockUserData);
        if (curTime - m_dwStartShowTime >= kShowDelayMilliseconds) {
            m_bShow = true;
        } else {
            return 0;  // WE_NULL (early return)
        }
    }
    return 0;  // WE_NULL
}

void cWantRegistDialog::Render() {
    if (m_bShow) {
        cDialog::Render();
    }
}

}  // namespace mxh::ui
