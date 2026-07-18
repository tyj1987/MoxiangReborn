// pointsavedialog.cpp — 1:1 port of 墨香
// CPointSaveDialog (map save-point name editor).
// See pointsavedialog.hpp for the data-model
// rationale + 1:1 quirks.

#include "pointsavedialog.hpp"
#include "ceditbox.hpp"
#include "ctextarea.hpp"

namespace mxh::ui {

cPointSaveDialog::cPointSaveDialog() {
    // 1:1 with legacy CPointSaveDialog ctor:
    //   m_bNewPoint = TRUE;
    //   m_ItemIdx = 0;
    //   m_ItemPos = 0;
    //
    // 1:1 quirk: modern bool uses default member
    // init (m_bNewPoint = true in header). The
    // ctor body is empty.
}

cPointSaveDialog::~cPointSaveDialog() = default;

void cPointSaveDialog::Linking() {
    // 1:1 with legacy CPointSaveDialog::Linking.
    // The legacy is:
    //   m_pNameEdtBox = (cEditBox*)GetWindowForID(CHA_NAMEEDITBOX);
    //   m_pNameEdtBox->SetValidCheck(VCM_CHARNAME);
    m_pNameEdtBox =
        static_cast<cEditBox*>(findWindowById(kIdNameEditBox));
    if (m_pNameEdtBox) {
        // 1:1 with legacy SetValidCheck(VCM_CHARNAME).
        m_pNameEdtBox->SetValidCheck(kVcmCharName);
    }
}

void cPointSaveDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CPointSaveDialog::SetActive
    // override. The legacy is:
    //   cDialog::SetActive(val);
    //   m_pNameEdtBox->SetFocusEdit(val);
    //   if (val) m_pNameEdtBox->SetEditText("");
    cDialog::SetActive(val);
    if (m_pNameEdtBox) {
        m_pNameEdtBox->SetFocusEdit(val);
        if (val) {
            m_pNameEdtBox->SetEditText("");
        }
    }
}

void cPointSaveDialog::SetItemToMapServer(std::uint32_t itemIdx,
                                         std::uint32_t itemPos) noexcept {
    // 1:1 with legacy SetItemToMapServer(DWORD, DWORD)
    // inline setter.
    m_ItemIdx = itemIdx;
    m_ItemPos = itemPos;
}

}  // namespace mxh::ui
