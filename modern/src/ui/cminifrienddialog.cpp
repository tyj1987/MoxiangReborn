// cminifrienddialog.cpp -- modern implementation of Moxiang CMiniFriendDialog.

#include "cminifrienddialog.hpp"

#include "cbutton.hpp"
#include "ceditbox.hpp"
#include "cstatic.hpp"

namespace mxh::ui {

cMiniFriendDialog::cMiniFriendDialog() = default;

cMiniFriendDialog::~cMiniFriendDialog() = default;

void cMiniFriendDialog::Linking() {
    // 1:1 with legacy CMiniFriendDialog::Linking.
    //   m_pName = (cStatic*)GetWindowForID(FRI_NAME);
    //   m_pNameEdit = (cEditBox*)GetWindowForID(FRI_NAMEEDIT);
    //   m_pNameEdit->SetValidCheck(VCM_CHARNAME);
    //   m_pNameEdit->SetEditText("");
    //   m_pAddOkBtn = (cButton*)GetWindowForID(FRI_ADDOKBTN);
    //   m_pAddCancelBtn = (cButton*)GetWindowForID(FRI_ADDCANCELBTN);
    // The modern port:
    //   * children resolved via host injection
    //     (SetChildrenForTest) -- the legacy
    //     GetWindowForID lookups are dropped
    //   * SetValidCheck(VCM_CHARNAME) is dropped
    //     (no modern text-validation helper; the host
    //     wires the validation policy in its own
    //     setup step)
    //   * SetEditText("") at Linking time is also
    //     dropped here (the legacy calls it once at
    //     Linking AND again at SetActive(true); the
    //     modern port defers to SetActive so the
    //     "open dialog = empty field" behaviour is
    //     preserved without double-clearing)
    //   (Note: legacy cEditBox::SetValidCheck is
    //   renamed to cEditBox::SetValidCharMode in
    //   modern; the cEditBox pointer carries a
    //   VCM_CHARNAME-equivalent state in m_validCharMode,
    //   but no port equivalent exists yet -- see
    //   KNOWN_BUGS R-12.x for the deferred work.)
}

void cMiniFriendDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CMiniFriendDialog::SetActive override.
    //   if( m_bDisable ) return;
    //   if(val)
    //       m_pNameEdit->SetEditText("");
    //   cDialog::SetActiveRecursive(val);
    if (m_bDisable) return;
    if (val && m_pNameEdit) {
        m_pNameEdit->SetEditText("");
    }
    cDialog::SetActiveRecursive(val);
}

void cMiniFriendDialog::SetName(const char* name) {
    // 1:1 with legacy CMiniFriendDialog::SetName(char* Name) --
    //   m_pNameEdit->SetEditText(Name);
    // The legacy crashes on null (raw strcpy).  The modern
    // port treats null as a safe no-op (defensive: the host
    // may call SetName after a failed lookup).
    if (m_pNameEdit && name) {
        m_pNameEdit->SetEditText(name);
    }
}

} // namespace mxh::ui
