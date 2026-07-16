// revivedialog.cpp — 1:1 port of 墨香 CReviveDialog (revive
// dialog). See revivedialog.hpp for the data-model rationale
// + 1:1 quirks.

#include "revivedialog.hpp"
#include "cbutton.hpp"

namespace mxh::ui {

cReviveDialog::cReviveDialog() = default;

cReviveDialog::~cReviveDialog() = default;

void cReviveDialog::Linking() {
    // 1:1 with legacy CReviveDialog::Linking. REAL — resolve
    // 3 cButton children by id. Defensive null-checks
    // (the legacy unconditionally dereferences each button
    // in SetActive).
    m_pPresentBtn = static_cast<cButton*>(findWindowById(kPresentBtnId));
    m_pLoginBtn   = static_cast<cButton*>(findWindowById(kLoginBtnId));
    m_pVillageBtn  = static_cast<cButton*>(findWindowById(kVillageBtnId));
}

void cReviveDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CReviveDialog::SetActive. The legacy
    // is:
    //   cDialog::SetActive(val);
    //   if (SIEGEMGR->GetSiegeWarMapNum() &&
    //       MAP->GetMapNum() == SIEGEMGR->GetSiegeWarMapNum()) {
    //       m_pPresentBtn->SetActive(FALSE);
    //       m_pVillageBtn->SetActive(TRUE);
    //   } else {
    //       m_pPresentBtn->SetActive(TRUE);
    //       m_pVillageBtn->SetActive(FALSE);
    //   }
    //
    // Modern port: call base SetActive first (1:1 with
    // legacy flow), then the button toggling is TODO
    // because SIEGEMGR + MAP singletons are not ported.
    cDialog::SetActive(val);
    // TODO: dispatch to SIEGEMGR + MAP once those singletons
    //       are ported. The button toggling branch is:
    //         if (SIEGEMGR->GetSiegeWarMapNum() &&
    //             MAP->GetMapNum() == SIEGEMGR->GetSiegeWarMapNum()) {
    //             // siege war: hide present, show village
    //             if (m_pPresentBtn) m_pPresentBtn->SetVisible(false);
    //             if (m_pVillageBtn) m_pVillageBtn->SetVisible(true);
    //         } else {
    //             // normal: show present, hide village
    //             if (m_pPresentBtn) m_pPresentBtn->SetVisible(true);
    //             if (m_pVillageBtn) m_pVillageBtn->SetVisible(false);
    //         }
    //
    // 1:1 quirk: the legacy calls button->SetActive(TRUE/FALSE),
    // but modern cButton doesn't have SetActive. The closest
    // equivalent is SetVisible (cWindow::SetVisible, inherited
    // by cButton). SetVisible(false) hides the button visually
    // and the user can't click it (the engine's input dispatch
    // skips invisible widgets). This is the same spirit as
    // the legacy SetActive(FALSE) on cButton — both result
    // in a non-interactive button.
    //
    // m_pLoginBtn is never toggled (1:1 quirk: legacy never
    // touches m_pLoginBtn in SetActive — login spot is
    // always available).
}

}  // namespace mxh::ui
