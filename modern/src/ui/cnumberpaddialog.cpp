// cnumberpaddialog.cpp — modern port of 墨香 CNumberPadDialog (PIN entry).
//
// 1:1 port of legacy `CNumberPadDialog::CNumberPadDialog` /
// `Linking` / `SetActive` / `OnActionEvent` / `InsertStr` /
// `InitProtectionStr` / `GetProtectionStr` from
//   `墨香【源码】\[Client]MH\NumberPadDialog.cpp`.

#include "mxh/ui/cnumberpaddialog.hpp"
#include "mxh/ui/ccombobox.hpp"
#include "mxh/ui/cstatic.hpp"

#include <cstring>

namespace mxh::ui {

cNumberPadDialog::cNumberPadDialog() {
    m_pStaticPN    = nullptr;
    m_pLogInWindow = nullptr;
    m_pCombo       = nullptr;
}

cNumberPadDialog::~cNumberPadDialog() = default;

void cNumberPadDialog::Linking() {
    // Legacy:
    //   m_pLogInWindow = WINDOWMGR->GetWindowForID(MT_LOGINDLG);
    //   m_pStaticPN    = (cStatic *)((cDialog*)m_pLogInWindow)->GetWindowForID(MT_PNSTATIC);
    //   m_pStaticPN->SetMultiLine(FALSE);
    //   m_pCombo       = (cComboBox*)WINDOWMGR->GetWindowForIDEx(MT_LISTCOMBOBOX);
    //   InitProtectionStr();
    //
    // Modern port: the host wires the pointers via SetStaticsForTest
    // (or the equivalent cWindowManager-driven path when the
    // window manager is fully integrated).  The legacy
    // SetMultiLine(FALSE) is a no-op in the modern port (cStatic
    // is single-line by default), so we just clear the buffer.
    InitProtectionStr();
}

void cNumberPadDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy SetActive(BOOL) which is a thin forwarder
    // to cDialog::SetActive.
    cDialog::SetActive(val);
}

bool cNumberPadDialog::OnActionEvent(std::int32_t lId, void* /*p*/, std::uint32_t we) {
    // 1:1 with legacy OnActionEvent:
    //   * WE_CLOSEWINDOW always returns TRUE (consume the event)
    //   * nGate == 3 short-circuits when the user is in gate 3
    //     (the legacy "supervisor" gate; PIN input is disabled)
    if (we == 0 /*WE_CLOSEWINDOW*/) {
        return true;
    }
    if (m_pCombo != nullptr && m_pCombo->GetCurSelectedIdx() == 3) {
        return true;
    }
    switch (lId) {
        case NUMBERPAD_BACKSPACE: InitProtectionStr();            break;
        case NUMBERPAD_0:          InsertStr("0");                  break;
        case NUMBERPAD_1:          InsertStr("1");                  break;
        case NUMBERPAD_2:          InsertStr("2");                  break;
        case NUMBERPAD_3:          InsertStr("3");                  break;
        case NUMBERPAD_4:          InsertStr("4");                  break;
        case NUMBERPAD_5:          InsertStr("5");                  break;
        case NUMBERPAD_6:          InsertStr("6");                  break;
        case NUMBERPAD_7:          InsertStr("7");                  break;
        case NUMBERPAD_8:          InsertStr("8");                  break;
        case NUMBERPAD_9:          InsertStr("9");                  break;
        default:                                                       break;
    }
    return true;
}

void cNumberPadDialog::InsertStr(const char* pStr) {
    if (m_pStaticPN == nullptr) return;
    const std::string& visible = m_pStaticPN->GetStaticText();
    const std::size_t nLen = visible.size();
    if (nLen >= 4) {
        // Legacy: nLen < 4 cap.  Once 4 digits are entered, additional
        // input is silently dropped (the legacy comment: "PIN 4
        // digits max").
        return;
    }
    // Append "*" to the visible static.
    std::string masked = visible;
    masked.push_back('*');
    m_pStaticPN->SetStaticText(masked);
    // Append the digit to the real buffer.
    if (pStr == nullptr) return;
    const std::size_t bufLen = std::strlen(m_pProtectionStr);
    const std::size_t dlen   = std::strlen(pStr);
    if (bufLen + dlen + 1 <= kProtectionStrMax) {
        std::memcpy(m_pProtectionStr + bufLen, pStr, dlen + 1);
    }
}

void cNumberPadDialog::InitProtectionStr() {
    if (m_pStaticPN != nullptr) {
        m_pStaticPN->SetStaticText("");
    }
    std::memset(m_pProtectionStr, 0, kProtectionStrMax);
}

std::size_t cNumberPadDialog::VisibleLength() const noexcept {
    return m_pStaticPN == nullptr ? 0u : m_pStaticPN->GetStaticText().size();
}

} // namespace mxh::ui
