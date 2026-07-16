// cmacrodialog.cpp — 1:1 port of 墨香 CMacroDialog (macro key
// binding dialog). See cmacrodialog.hpp for the data-model
// rationale + 1:1 quirks.

#include "cmacrodialog.hpp"
#include "ceditbox.hpp"

#include <algorithm>
#include <cstring>
#include <string>

namespace mxh::ui {

cMacroDialog::cMacroDialog() = default;

cMacroDialog::~cMacroDialog() = default;

void cMacroDialog::Init(std::int32_t x, std::int32_t y,
                        std::uint16_t wid, std::uint16_t hei) {
    // 1:1 with legacy CMacroDialog::Init(x, y, wid, hei,
    // basicImage, ID). The modern port omits the basicImage
    // argument — the dialog's background image is added as a
    // separate cStatic / cImage child by the host, not baked
    // into Init. The ID is also omitted; the host calls
    // cDialog::SetID() before adding the dialog to the tree.
    cDialog::Init(x, y, wid, hei, /*basicImage=*/nullptr, /*ID=*/0);
    // 1:1 quirk: legacy m_nMode defaults to 0 (Chat mode);
    // m_MacroKey array is value-initialized to all-zero
    // (nSysKey = MSK_NONE = 1, wKey = 0, bAllMode = false,
    // bUp = false) via std::array<...>{} default construction.
    m_MacroKey.fill(sMACRO{});
    m_pMacroKeyEdit.fill(nullptr);
    m_nMode     = static_cast<std::uint8_t>(MacroMode::Chat);
    m_bChanged  = false;
    m_pFocusEdit = nullptr;
    m_nCurMacro = -1;
}

void cMacroDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CMacroDialog::SetActive(BOOL val).
    // The legacy gates refresh-on-show on m_nMode (m_bActive
    // only triggers ConvertMacroToText when in Macro mode).
    // The modern port mirrors this gate so the visible text
    // matches what the legacy would show.
    cDialog::SetActive(val);
    if (val && m_nMode == static_cast<std::uint8_t>(MacroMode::Macro)) {
        for (std::size_t i = 0; i < kMacroCount; ++i) {
            if (m_pMacroKeyEdit[i]) {
                char buf[kMaxMacroTextLen + 1] = {};
                ConvertMacroToText(buf, m_MacroKey[i]);
                m_pMacroKeyEdit[i]->SetEditText(buf);
            }
        }
    }
}

void cMacroDialog::Linking() {
    // 1:1 with legacy CMacroDialog::Linking(). The legacy
    // hardcodes 15 (cEditBox*)GetWindowForID(MAC_EB_*) calls
    // and stores them in m_pMacroKeyEdit[ME_*]. The modern
    // port uses a sequential id range (the host assigns ids
    // MAC_EB_QUICKITEM01 = 100, MAC_EB_QUICKITEM02 = 101,
    // etc.; see docs/WindowIDEnum.md for the full id table
    // when the WindowIDEnum.h port lands).
    //
    // The id assignment is:
    //   100 + i  for the i-th quick-slot binding (0..6)
    //   200 + i  for the toggle-dialog bindings (0..10,
    //             starting at TOGGLE_INVENTORYDLG)
    //   300 + i  for the mode bindings (0..5,
    //             starting at TOGGLE_MOVEMODE)
    //   400 + 0  for TOGGLE_MINIMAP
    //   400 + 1  for TOGGLE_CAMERAVIEWMODE
    //   500 + 0  for SCREENCAPTURE
    //
    // The host can override these by setting the cEditBox id
    // explicitly before calling Linking().
    auto resolveEdit = [this](MacroEvent evt) -> cEditBox* {
        std::int32_t id = -1;
        const auto e = static_cast<std::uint8_t>(evt);
        if (e >= static_cast<std::uint8_t>(MacroEvent::USE_QUICKITEM01) &&
            e <= static_cast<std::uint8_t>(MacroEvent::USE_QUICKITEM07)) {
            id = 100 + e;
        } else if (e >= static_cast<std::uint8_t>(MacroEvent::TOGGLE_INVENTORYDLG) &&
                   e <= static_cast<std::uint8_t>(MacroEvent::TOGGLE_CAMERAVIEWMODE)) {
            id = 200 + (e - static_cast<std::uint8_t>(MacroEvent::TOGGLE_INVENTORYDLG));
        } else if (e == static_cast<std::uint8_t>(MacroEvent::SCREENCAPTURE)) {
            id = 500;
        } else {
            return nullptr;
        }
        return static_cast<cEditBox*>(findWindowById(id));
    };
    for (std::size_t i = 0; i < kMacroCount; ++i) {
        m_pMacroKeyEdit[i] = resolveEdit(static_cast<MacroEvent>(i));
        if (m_pMacroKeyEdit[i]) {
            m_pMacroKeyEdit[i]->ShowCaretInReadOnly(true);
        }
    }
    m_pFocusEdit = m_pMacroKeyEdit[0];
}

void cMacroDialog::SetMacroBinding(MacroEvent evt, const sMACRO& macro) {
    const auto idx = static_cast<std::size_t>(evt);
    if (idx >= kMacroCount) return;
    m_MacroKey[idx] = macro;
    m_bChanged = true;
    if (m_pMacroKeyEdit[idx]) {
        char buf[kMaxMacroTextLen + 1] = {};
        ConvertMacroToText(buf, m_MacroKey[idx]);
        m_pMacroKeyEdit[idx]->SetEditText(buf);
    }
}

sMACRO cMacroDialog::GetMacroBinding(MacroEvent evt) const {
    const auto idx = static_cast<std::size_t>(evt);
    if (idx >= kMacroCount) return sMACRO{};
    return m_MacroKey[idx];
}

std::size_t cMacroDialog::ConvertMacroToText(char* text, const sMACRO& macro) const {
    // 1:1 with legacy CMacroDialog::ConvertMacroToText. The
    // legacy uses a long STARTCONVERT/CONVERT/ENDCONVERT macro
    // expansion; the modern port uses a flat switch on the
    // modifier (only MSK_CTRL / MSK_ALT / MSK_SHIFT are
    // enumerated; MSK_NONE skips the prefix).
    if (!text) return 0;
    text[0] = '\0';
    switch (static_cast<SysKey>(macro.nSysKey)) {
        case SysKey::Ctrl:  std::strcat(text, "CTRL + ");  break;
        case SysKey::Alt:   std::strcat(text, "ALT + ");   break;
        case SysKey::Shift: std::strcat(text, "SHIFT + "); break;
        case SysKey::None:
        case SysKey::All:
        default:            /* no prefix */                break;
    }
    const std::string name = VKeyToName(macro.wKey);
    if (!name.empty()) {
        std::strcat(text, name.c_str());
    }
    return std::strlen(text);
}

std::string cMacroDialog::VKeyToName(std::uint16_t vk) {
    // 1:1 with legacy ConvertMacroToText's inner switch on
    // pMacro->wKey. The legacy enumerates ~50 VK codes; the
    // modern port covers the most common ones. VK codes that
    // aren't recognized return an empty string (the dialog
    // will show just the modifier, e.g. "CTRL + " — the host
    // can extend VKeyToName with more codes when needed).
    switch (vk) {
        // Function keys.
        case 0x70: return "F1";   case 0x71: return "F2";
        case 0x72: return "F3";   case 0x73: return "F4";
        case 0x74: return "F5";   case 0x75: return "F6";
        case 0x76: return "F7";   case 0x77: return "F8";
        case 0x78: return "F9";   case 0x79: return "F10";
        case 0x7A: return "F11";  case 0x7B: return "F12";
        // Letters (VK_A..VK_Z = 0x41..0x5A).
        case 0x41: return "A";    case 0x42: return "B";
        case 0x43: return "C";    case 0x44: return "D";
        case 0x45: return "E";    case 0x46: return "F";
        case 0x47: return "G";    case 0x48: return "H";
        case 0x49: return "I";    case 0x4A: return "J";
        case 0x4B: return "K";    case 0x4C: return "L";
        case 0x4D: return "M";    case 0x4E: return "N";
        case 0x4F: return "O";    case 0x50: return "P";
        case 0x51: return "Q";    case 0x52: return "R";
        case 0x53: return "S";    case 0x54: return "T";
        case 0x55: return "U";    case 0x56: return "V";
        case 0x57: return "W";    case 0x58: return "X";
        case 0x59: return "Y";    case 0x5A: return "Z";
        // Digits.
        case 0x30: return "0";    case 0x31: return "1";
        case 0x32: return "2";    case 0x33: return "3";
        case 0x34: return "4";    case 0x35: return "5";
        case 0x36: return "6";    case 0x37: return "7";
        case 0x38: return "8";    case 0x39: return "9";
        // Common controls.
        case 0x20: return "SPACE";  case 0x09: return "TAB";
        case 0x0D: return "ENTER";  case 0x1B: return "ESC";
        case 0x08: return "BACKSPACE";
        // Arrows.
        case 0x25: return "LEFT";   case 0x26: return "UP";
        case 0x27: return "RIGHT";  case 0x28: return "DOWN";
        case 0x21: return "PGUP";   case 0x22: return "PGDN";
        case 0x24: return "HOME";   case 0x23: return "END";
        case 0x2D: return "INSERT"; case 0x2E: return "DEL";
        default: return "";
    }
}

}  // namespace mxh::ui
