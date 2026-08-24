// petstateminidlg.hpp — modern port of 墨香 CPetStateMiniDlg (pet state UI).
//
// 1:1 port of legacy `CPetStateMiniDlg` from
//   `墨香【源码】\[Client]MH\PetStateMiniDlg.{h,cpp}`.
//
// CPetStateMiniDlg is the mini pet state UI: shows the pet's name,
// state (use / rest), friendship level, stamina, and 3 toggle buttons
// (use/rest, inventory, expand-to-full-pet-state-dialog). The legacy
// class wires 4 cStatic labels + 2 cGuagen gauges + 3 cButton buttons
// (9 children total) and routes button clicks through the PETMGR
// global singleton.
//
// 1:1 contract preserved:
//   - 9 child windows, all resolved by id at Linking() time. The
//     legacy uses `GetWindowForID(PSMN_*)`; modern port uses
//     `std::make_unique<>` + a local id range (800-808).
//   - OnActionEvent dispatches 3 button clicks to the PETMGR global
//     singleton (TogglePetStateDlg / SendPetRestMsg / OpenPetInvenDlg).
//   - Linking is idempotent (re-callable without re-creating children).
//   - 6 read accessors (GetNameTextWin / GetUseRestTextWin /
//     GetFriendShipTextWin / GetStaminaTextWin / GetFriendShipGuage /
//     GetStaminaGuage) — 1:1 with legacy cStatic/cGuagen raw accessors.
//
// 1:1 quirks preserved:
//   - 1:1 quirk: legacy ctor zeroes 8 raw pointers to NULL. Modern
//     port uses std::unique_ptr (nullptr by default) — no explicit
//     zero-init required.
//   - 1:1 quirk: legacy `OnActionEvent` is no-op for non-button ids
//     (no early return, no logging). Modern port matches.
//   - 1:1 quirk: legacy `PSMN_USEREST_BTN` branch checks
//     `NULL == PETMGR->GetCurSummonPet()` and returns. Modern port
//     preserves this short-circuit (the PETMGR stub returns nullptr
//     from GetCurSummonPet, so the branch always returns — same
//     effect as legacy when no pet is summoned).
//   - 1:1 quirk: legacy `PSMN_TOGGLE_B_BTN` always toggles (no
//     precondition). Modern port matches.
//   - 1:1 quirk: legacy `PSMN_INVEN_BTN` always opens (no
//     precondition). Modern port matches.
//   - 1:1 quirk: legacy uses raw pointer `cStatic*` / `cGuagen*` /
//     `cButton*` for members. Modern port uses std::unique_ptr<T>
//     (per Phase 6 ownership semantics).
//   - 1:1 quirk: legacy `OnActionEvent` is not virtual. Modern port
//     matches (cDialog's ActionEvent is virtual, but OnActionEvent
//     is the host-app dispatch seam and is not overridden here).
//   - 1:1 quirk: PETMGR global singleton is stubbed (no-op emitter
//     for the 3 calls). Host app can wire a real PETMGR by setting
//     the global before invoking OnActionEvent.

#pragma once

#include "cDialog.hpp"

#include <cstdint>
#include <memory>

namespace mxh::ui {

class cStatic;
class cButton;
class cGuagen;

class cPetStateMiniDlg : public cDialog {
public:
    // Local id range (1:1 with legacy PSMN_* values; modern port uses
    // 800-808 to avoid clashes with previously-ported dialogs).
    static constexpr int kIdName        = 800;
    static constexpr int kIdState       = 801;
    static constexpr int kIdFriend      = 802;
    static constexpr int kIdStamina     = 803;
    static constexpr int kIdFriendGuage = 804;
    static constexpr int kIdStaminaGuage = 805;
    static constexpr int kIdUseRestBtn  = 806;
    static constexpr int kIdInvenBtn    = 807;
    static constexpr int kIdToggleBtn   = 808;

    cPetStateMiniDlg();
    ~cPetStateMiniDlg() override;

    cPetStateMiniDlg(const cPetStateMiniDlg&) = delete;
    cPetStateMiniDlg& operator=(const cPetStateMiniDlg&) = delete;

    void Linking();
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // 1:1 with legacy raw-pointer accessors.
    cStatic* GetNameTextWin()       const noexcept { return m_pNameDlg.get(); }
    cStatic* GetUseRestTextWin()    const noexcept { return m_pStateDlg.get(); }
    cStatic* GetFriendShipTextWin() const noexcept { return m_pFriend.get(); }
    cStatic* GetStaminaTextWin()    const noexcept { return m_pStamina.get(); }
    cGuagen*  GetFriendShipGuage()  const noexcept { return m_pFriendGuage.get(); }
    cGuagen*  GetStaminaGuage()     const noexcept { return m_pStaminaGuage.get(); }

    // Test accessors for buttons.
    cButton* GetUseRestButton() const noexcept { return m_pPetUseRestBtn.get(); }
    cButton* GetInvenButton()   const noexcept { return m_pPetInvenBtn.get(); }
    cButton* GetToggleButton()  const noexcept { return m_pDlgToggleBtn.get(); }

private:
    std::unique_ptr<cStatic> m_pNameDlg;        // legacy: name label
    std::unique_ptr<cStatic> m_pStateDlg;       // legacy: state label
    std::unique_ptr<cStatic> m_pFriend;         // legacy: friendship label
    std::unique_ptr<cStatic> m_pStamina;        // legacy: stamina label
    std::unique_ptr<cGuagen> m_pFriendGuage;    // legacy: friendship guage
    std::unique_ptr<cGuagen> m_pStaminaGuage;   // legacy: stamina guage
    std::unique_ptr<cButton> m_pPetUseRestBtn;  // legacy: use/rest button
    std::unique_ptr<cButton> m_pPetInvenBtn;    // legacy: inventory button
    std::unique_ptr<cButton> m_pDlgToggleBtn;   // legacy: expand-to-full button
};

} // namespace mxh::ui
