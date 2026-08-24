// maindialog.hpp — modern port of 墨香
// CMainDialog (main UI button bar dialog:
// 4 cPushupButton captured via Add() override).
//
// 1:1 port of legacy `CMainDialog` from
//   `墨香【源码】\[Client]MH\MainDialog.h` (1111 B) and
//   `墨香【源码】\[Client]MH\MainDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_MAINDIALOG; zero-initialize
//     m_pBtn[MAX_BTN=5] (only 4 are populated: CHAR_BTN,
//     INVENTORY_BTN, MUGONG_BTN, PARTY_BTN).
//   - Dtor: empty body.
//   - Add(cWindow*) override: 4 branch capture — if !m_pBtn[idx]
//     && window->GetID() == MI_XxxBTN, store
//     (cPushupButton*)window. Always call cDialog::Add
//     afterwards (pass-through ownership).
//   - GetPushupBtn(WORD idx): return m_pBtn[idx].
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_type = WT_MAINDIALOG
//     drop, modern cWindow does not have m_type).
//   - Dtor: empty (no-op).
//   - Linking: REAL — synth 4 cPushupButton with the
//     right id range + store them. 1:1 quirk: legacy
//     uses Add() side-channel to capture cPushupButton
//     references from the resource loader; modern port
//     synthesizes the children in Linking (no resource
//     loader hook needed in modern).
//   - GetPushupBtn: REAL (1:1 with legacy).
//   - 1:1 quirk: legacy ctor zero-initializes m_pBtn[5]
//     but only 4 are used; modern port inlines
//     m_pBtn[4] (4 unique_ptr) and ignores MAX_BTN=5.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 39th **Tier 2** dialog port (after
// cWantRegistDialog). The dialog has no singleton
// dependency — all 4 cPushupButton are owned by the
// dialog itself (R-12.x resource loader is the only
// missing piece, replaced by Linking-side synth).
//
// Note on the legacy Add() override: the legacy
// CMainDialog overrides cDialog::Add(cWindow*) to
// capture cPushupButton references as the resource
// loader adds them. The modern cWindow::Add is
// non-virtual (takes unique_ptr<cWindow> by value),
// so cMainDialog cannot override Add. The modern
// port provides Linking() as the equivalent
// capture point — cPushupButton are created in
// Linking() rather than captured from a resource
// loader side-channel. This is a 1:1 *semantic*
// preservation: the post-Linking state has the
// same 4 cPushupButton captured.

#pragma once

#include "cdialog.hpp"

#include <cstdint>
#include <memory>

namespace mxh::ui {

class cPushupButton;

class cMainDialog : public cDialog {
public:
    cMainDialog();
    ~cMainDialog() override;

    // ----- 1:1 with legacy CMainDialog::Linking -----

    // 1:1 with legacy Linking. Synthesizes 4
    // cPushupButton (CHAR / INVENTORY / MUGONG / PARTY)
    // and stores them in m_pBtn. The id constants
    // below match legacy WindowIDs.h MI_INVENTORYBTN
    // / MI_MUGONGBTN / MI_CHARBTN / MI_PARTYBTN
    // (530-533).
    void Linking();

    // ----- 1:1 with legacy CMainDialog::GetPushupBtn -----

    // 1:1 with legacy GetPushupBtn(WORD idx). Returns
    // m_pBtn[idx] or nullptr if not populated.
    cPushupButton* GetPushupBtn(std::uint16_t idx) const noexcept;

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h MI_INVENTORYBTN /
    // MI_MUGONGBTN / MI_CHARBTN / MI_PARTYBTN (530-533).
    // Local 530-533 — distinct from 200-520 used by
    // previous Tier 2 dialogs.
    static constexpr std::int32_t kIdInventoryBtn = 530;
    static constexpr std::int32_t kIdMugongBtn    = 531;
    static constexpr std::int32_t kIdCharBtn      = 532;
    static constexpr std::int32_t kIdPartyBtn     = 533;

    // 1:1 with legacy CHAR_BTN=0 / INVENTORY_BTN=1 /
    // MUGONG_BTN=2 / PARTY_BTN=4 (skip 3). Modern
    // port inlines only the 4 valid indices, in
    // insertion order: kIdxChar=0, kIdxInventory=1,
    // kIdxMugong=2, kIdxParty=3.
    static constexpr std::int32_t kIdxChar      = 0;
    static constexpr std::int32_t kIdxInventory = 1;
    static constexpr std::int32_t kIdxMugong    = 2;
    static constexpr std::int32_t kIdxParty     = 3;
    static constexpr std::int32_t kNumBtns      = 4;

private:
    // 1:1 with legacy m_pBtn[MAX_BTN=5] (only 4
    // populated). Modern port uses 4 unique_ptr in
    // the 1:1 quirk order (CHAR / INVENTORY /
    // MUGONG / PARTY).
    std::unique_ptr<cPushupButton> m_pBtn[kNumBtns];
};

}  // namespace mxh::ui
