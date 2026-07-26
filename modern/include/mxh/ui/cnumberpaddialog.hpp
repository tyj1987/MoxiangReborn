// cnumberpaddialog.hpp — modern port of 墨香 CNumberPadDialog (PIN entry).
//
// 1:1 port of legacy `CNumberPadDialog` from
//   `墨香【源码】\[Client]MH\NumberPadDialog.{h,cpp}`.
//
// The legacy dialog is a small 0-9 + backspace PIN keypad.  The
// static text shown in the UI is a 4-character "*" mask; the real
// PIN lives in m_pProtectionStr and is exposed via GetProtectionStr().
//
// 1:1 dependencies:
//   * cStatic for the masked display
//   * cComboBox for the "gate" selector (legacy nGate == 3 short-circuit)
//   * cWindow for the parent login dialog (cDialog lookup)
//
// Modern port keeps the legacy surface (InsertStr / InitProtectionStr /
// GetProtectionStr / OnActionEvent(NUMBERPAD_* ids)) so callers can be
// ported 1:1.  The host wires up:
//   - the cStatic (typically the login dialog's static)
//   - the cComboBox (gate selector)
// via Linking(); the host calls OnActionEvent with the legacy WE_BTNCLICK
// + button id (NUMBERPAD_0..9, NUMBERPAD_BACKSPACE) flow.

#pragma once

#include "cDialog.hpp"

#include <cstdint>
#include <cstring>

namespace mxh::ui {

// Legacy button ids (1:1 with NumberPadDialog.cpp::OnActionEvent).
// Mirrors the WindowIDEnum entries used by the resource script.
enum NumberPadButton : std::int32_t {
    NUMBERPAD_BACKSPACE = 0,
    NUMBERPAD_0 = 1,
    NUMBERPAD_1 = 2,
    NUMBERPAD_2 = 3,
    NUMBERPAD_3 = 4,
    NUMBERPAD_4 = 5,
    NUMBERPAD_5 = 6,
    NUMBERPAD_6 = 7,
    NUMBERPAD_7 = 8,
    NUMBERPAD_8 = 9,
    NUMBERPAD_9 = 10,
    NUMBERPAD_MAX = 11,
};

class cStatic;
class cComboBox;

class cNumberPadDialog : public cDialog {
public:
    // Legacy: ePROTECTIONSTR_MAXNUM = 10 (raw buffer size, char[10]).
    // The PIN is capped at 4 displayed characters but the buffer
    // holds 10 to match the legacy struct footprint.
    static constexpr std::size_t kProtectionStrMax = 10;

    cNumberPadDialog();
    ~cNumberPadDialog() override;

    cNumberPadDialog(const cNumberPadDialog&) = delete;
    cNumberPadDialog& operator=(const cNumberPadDialog&) = delete;

    // 1:1 with legacy Linking().  Looks up the parent login dialog
    // via the window manager, grabs the static that shows the masked
    // PIN, and grabs the gate cComboBox.  The host caller is
    // responsible for ensuring MT_LOGINDLG / MT_PNSTATIC / MT_LISTCOMBOBOX
    // are registered with the window manager before Linking runs.
    void Linking();

    // cDialog::SetActive override -- the legacy dialog is a no-op
    // forwarder (cDialog::SetActive).  Kept for 1:1 surface.
    void SetActive(bool val) noexcept override;

    // 1:1 with legacy OnActionEvent.  Returns TRUE if the event was
    // consumed (which is always in the legacy flow except when the
    // gate is the short-circuit value -- nGate == 3).  Hosts forward
    // WE_BTNCLICK events here from cPushupButton (or cButton) clicks
    // on the 0-9 / backspace keys.
    bool OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // 1:1 with legacy InsertStr.  Appends "*" to the visible masked
    // static and the digit to the real buffer.  Caps the visible
    // length at 4 chars (legacy behaviour: nLen < 4).
    void InsertStr(const char* pStr);

    // 1:1 with legacy InitProtectionStr.  Clears the static text
    // and zero-fills the real buffer.
    void InitProtectionStr();

    // 1:1 with legacy GetProtectionStr.  Returns the raw buffer
    // (not the visible mask) so the caller can pass it to the
    // login network layer.
    const char* GetProtectionStr() const noexcept { return m_pProtectionStr; }

    // Test accessor.
    std::size_t VisibleLength() const noexcept;

    // Test hook -- replace the underlying cStatic / cComboBox
    // pointers (avoiding the full cWindowManager + MT_LOGINDLG
    // dance in unit tests).  Caller owns the storage.
    void SetStaticsForTest(cStatic* stat, cComboBox* combo) noexcept {
        m_pStaticPN = stat;
        m_pCombo   = combo;
    }

private:
    char      m_pProtectionStr[kProtectionStrMax] = {};
    cStatic*  m_pStaticPN  = nullptr;
    cWindow*  m_pLogInWindow = nullptr;   // unused on modern; kept for 1:1 surface
    cComboBox* m_pCombo    = nullptr;
};

} // namespace mxh::ui
