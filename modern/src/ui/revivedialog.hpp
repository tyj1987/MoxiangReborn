// revivedialog.hpp — modern port of 墨香 CReviveDialog
// (revive dialog: 3 options — revive at present spot, login
// spot, or village).
//
// 1:1 port of legacy `CReviveDialog` from
//   `墨香【源码】\[Client]MH\ReviveDialog.h` (729 B) and
//   `墨香【源码】\[Client]MH\ReviveDialog.cpp`.
//
// What the legacy does:
//   - Linking: resolve 3 cButton children by id (CR_PRESENTSPOT
//     / CR_LOGINSPOT / CR_TOWNSPOT) — pure widget ops, no
//     singleton.
//   - SetActive override: calls cDialog::SetActive(val), then
//     checks SIEGEMGR->GetSiegeWarMapNum() + MAP->GetMapNum():
//       * If current map is a siege war map:
//         m_pPresentBtn->SetActive(FALSE);  // can't revive
//                                            // at present
//                                            // spot during siege
//         m_pVillageBtn->SetActive(TRUE);   // force village
//       * Else (normal map):
//         m_pPresentBtn->SetActive(TRUE);   // present spot
//                                            // available
//         m_pVillageBtn->SetActive(FALSE);  // login spot
//                                            // only
//     The m_pLoginBtn is never toggled by SetActive (it's
//     always shown — the user can always choose login spot).
//
// The modern port covers Linking (REAL — 3 button resolutions
// are pure widget ops). SetActive override matches the base
// noexcept spec (R-12 polymorphic virtual) and calls the base
// SetActive first. The button active/inactive toggling is
// TODO until SIEGEMGR + MAP singletons are ported.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this
// is the 9th **Tier 2** dialog port (after cExitDialog,
// cMacroDialog, cCharMakeDlg, cGuildJoinDialog,
// cCharStateDialog, cSOSDialog, cWearedExDialog,
// cMiniFriendDialog). The dialog has 3 cButton children
// (all already ported). The SIEGEMGR + MAP singletons
// are tracked as future Tier 3+ work items.
//
// 1:1 quirks preserved:
//   - SetActive matches base noexcept (R-12 polymorphic
//     virtual required).
//   - SetActive calls base first, then the button toggling
//     (1:1 with legacy: cDialog::SetActive runs before the
//     SIEGEMGR check).
//   - m_pLoginBtn is not toggled by SetActive (1:1 quirk:
//     legacy never touches m_pLoginBtn in SetActive).
//   - The button toggling (SetActive FALSE/TRUE) is documented
//     in the TODO. The modern cButton doesn't have SetActive
//     (the legacy cButton did); modern port would use
//     SetVisible to express the same intent. This is
//     documented as a 1:1 quirk in the cpp.

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cButton;

class cReviveDialog : public cDialog {
public:
    cReviveDialog();
    ~cReviveDialog() override;

    // ----- 1:1 with legacy CReviveDialog::Linking -----

    // Resolves 3 cButton children by id (kPresentBtnId=250,
    // kLoginBtnId=251, kVillageBtnId=252). Pure widget ops.
    void Linking();

    // ----- 1:1 with legacy CReviveDialog::SetActive -----

    // 1:1 override: calls base SetActive(val), then the
    // button toggling based on SIEGEMGR + MAP singletons.
    // The singleton dispatch is TODO (SIEGEMGR + MAP not
    // ported yet). The button toggling (m_pPresentBtn /
    // m_pVillageBtn visibility) is also TODO.
    void SetActive(bool val) noexcept override;

    // ----- Accessors (used by tests + future singleton bridge) -----

    cButton* GetPresentBtn() const noexcept { return m_pPresentBtn; }
    cButton* GetLoginBtn()   const noexcept { return m_pLoginBtn; }
    cButton* GetVillageBtn()  const noexcept { return m_pVillageBtn; }

    // ----- Local id range (matches modern test convention) -----

    static constexpr std::int32_t kPresentBtnId = 250;  // was CR_PRESENTSPOT
    static constexpr std::int32_t kLoginBtnId   = 251;  // was CR_LOGINSPOT
    static constexpr std::int32_t kVillageBtnId  = 252;  // was CR_TOWNSPOT

private:
    cButton* m_pPresentBtn = nullptr;
    cButton* m_pLoginBtn   = nullptr;
    cButton* m_pVillageBtn  = nullptr;
};

}  // namespace mxh::ui
