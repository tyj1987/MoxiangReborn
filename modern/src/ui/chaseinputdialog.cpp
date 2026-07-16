// chaseinputdialog.cpp — 1:1 port of 墨香 CChaseinputDialog
// (chase input dialog). See chaseinputdialog.hpp for the
// data-model rationale + 1:1 quirks.

#include "chaseinputdialog.hpp"
#include "ceditbox.hpp"

namespace mxh::ui {

cChaseInputDialog::cChaseInputDialog() = default;

cChaseInputDialog::~cChaseInputDialog() = default;

void cChaseInputDialog::Linking() {
    // 1:1 with legacy CChaseinputDialog::Linking. REAL
    // — resolve cEditBox + SetValidCheck. Defensive
    // null-checks (the legacy unconditionally
    // dereferences m_pEditName in SetValidCheck /
    // SetEditText).
    m_pEditName = static_cast<cEditBox*>(findWindowById(kEditNameId));
    if (m_pEditName) {
        // 1:1 quirk: legacy calls
        //   m_pEditName->SetValidCheck(VCM_CHARNAME)
        // where VCM_CHARNAME = 2 (from cIMEex.h, the
        // character-name validator enum). The modern
        // cEditBox supports 0/1/2/3 modes; closest
        // modern equivalent is mode 2 (alpha only).
        // Modern port uses kVcmCharnameAlias = 2.
        m_pEditName->SetValidCheck(kVcmCharnameAlias);
    }
}

void cChaseInputDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CChaseinputDialog::SetActive.
    // The legacy is:
    //   cDialog::SetActive(val);
    //   if (val) {
    //       m_pEditName->SetEditText("");
    //       m_dwItemIdx = 0;
    //   }
    //
    // Modern port: call base SetActive first, then
    // the if val branch.
    cDialog::SetActive(val);
    if (val && m_pEditName) {
        m_pEditName->SetEditText("");
        m_dwItemIdx = 0;
    }
}

void cChaseInputDialog::WantedChaseSyn() {
    // 1:1 with legacy CChaseinputDialog::WantedChaseSyn.
    // The legacy is a 6-singleton dispatch:
    //   1. Rate limit (gCurTime - m_LastChktime < 30000)
    //      → CHATMGR->AddMsg(909) + return
    //   2. Read m_pEditName text, copy to buf + buftmp
    //   3. strlen(buf) == 0 → return
    //   4. buf == HERO->GetObjectName() → CHATMGR(911) + return
    //   5. FILTERTABLE->FilterWordInString → CHATMGR(919) + return
    //   6. m_dwItemIdx == eIncantation_Tracking_Jin check
    //   7. NETWORK->Send(MP_ITEM_SHOPITEM_CHASE_SYN, ...)
    //   8. SetActive(FALSE) + m_LastChktime = gCurTime
    //
    // Modern port: TODO until gCurTime + CHATMGR + HERO
    // + FILTERTABLE + WANTEDMGR + NETWORK singletons
    // are ported.
    (void)0;
    // TODO: implement the 6-singleton dispatch once
    //       the global singletons are ported.
}

}  // namespace mxh::ui
