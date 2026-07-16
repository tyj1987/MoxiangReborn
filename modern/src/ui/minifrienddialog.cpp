// minifrienddialog.cpp — 1:1 port of 墨香 CMiniFriendDialog
// (mini friend-add dialog). See minifrienddialog.hpp for
// the data-model rationale + 1:1 quirks.

#include "minifrienddialog.hpp"
#include "cstatic.hpp"
#include "ceditbox.hpp"
#include "cbutton.hpp"

namespace mxh::ui {

cMiniFriendDialog::cMiniFriendDialog() = default;

cMiniFriendDialog::~cMiniFriendDialog() = default;

void cMiniFriendDialog::Init(std::int32_t x, std::int32_t y,
                             std::uint16_t wid, std::uint16_t hei,
                             void* basicImage, std::int32_t id) {
    // 1:1 with legacy CMiniFriendDialog::Init. The legacy
    // is:
    //   cDialog::Init(x, y, wid, hei, basicImage, ID);
    //   m_type = WT_MINIFRIENDDLG;
    //
    // Modern cDialog::Init has the same signature, so the
    // base call is identical. The m_type assignment is
    // dropped (1:1 quirk: legacy cWindow type tag removed
    // in Phase 6 — modern cWindow / cDialog don't have
    // m_type).
    cDialog::Init(x, y, wid, hei, basicImage, id);
}

void cMiniFriendDialog::Linking() {
    // 1:1 with legacy CMiniFriendDialog::Linking. REAL —
    // resolves 4 children by id and configures
    // m_pNameEdit (SetValidCheck + clear text). Defensive
    // null-checks (the legacy unconditionally
    // dereferences m_pNameEdit in SetValidCheck /
    // SetEditText).
    m_pName         = static_cast<cStatic*>(findWindowById(kNameId));
    m_pNameEdit     = static_cast<cEditBox*>(findWindowById(kNameEditId));
    m_pAddOkBtn     = static_cast<cButton*>(findWindowById(kAddOkBtnId));
    m_pAddCancelBtn = static_cast<cButton*>(findWindowById(kAddCancelBtnId));

    if (m_pNameEdit) {
        // 1:1 quirk: legacy calls
        //   m_pNameEdit->SetValidCheck(VCM_CHARNAME)
        // where VCM_CHARNAME = 2 (from cIMEex.h, the
        // character-name validator enum). The modern
        // cEditBox supports 0=none, 1=digits only,
        // 2=alpha only, 3=alnum. The closest modern
        // equivalent for VCM_CHARNAME is mode 2 (alpha
        // only). The legacy VCM_CHARNAME includes
        // cIMEex integration (which isn't ported) but
        // the validation mode matches modern mode 2.
        // Modern port uses kVcmCharnameAlias (= 2) as
        // the alias.
        m_pNameEdit->SetValidCheck(kVcmCharnameAlias);
        m_pNameEdit->SetEditText("");
    }
}

void cMiniFriendDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CMiniFriendDialog::SetActive. The
    // legacy is:
    //   if (m_bDisable) return;
    //   if (val) m_pNameEdit->SetEditText("");
    //   cDialog::SetActiveRecursive(val);
    //
    // 1:1 quirk: legacy checks m_bDisable. Modern
    // cWindow has m_bEnabled (the inverse flag) and
    // exposes isEnabled() / SetEnabled() / SetDisable().
    // The 1:1 flow is "if (m_bDisable) return;" —
    // i.e., "if disabled, return" — which maps to
    // "if (!isEnabled()) return;".
    if (!isEnabled()) return;
    if (val && m_pNameEdit) {
        m_pNameEdit->SetEditText("");
    }
    SetActiveRecursive(val);
}

void cMiniFriendDialog::SetName(const char* name) {
    // 1:1 with legacy CMiniFriendDialog::SetName. The
    // legacy is:
    //   void CMiniFriendDialog::SetName(char* Name) {
    //       m_pNameEdit->SetEditText(Name);
    //   }
    //
    // Defensive null-check (the legacy unconditionally
    // dereferences).
    if (m_pNameEdit) m_pNameEdit->SetEditText(name);
}

}  // namespace mxh::ui
