// crevivedialog.hpp -- modern port of Moxiang CReviveDialog (revive options).
//
// 1:1 port of legacy `CReviveDialog` from
//   `[Client]MH\ReviveDialog.{h,cpp}`.
//
// The revive dialog is a 3-button prompt shown after the
// player dies: present-spot, login-spot, or village-spot.
// In siege-war maps, the village button is shown instead
// of the present button (siege rules).
//
// 1:1 dependencies:
//   * 3 cButton children (Present / Login / Village)
//   * SIEGEMGR->GetSiegeWarMapNum() + MAP->GetMapNum()
//     for the siege-war branch (modern port routes
//     both lookups through host-injected callbacks)
//
// Modern port keeps the legacy surface (Linking +
// SetActive override) so callers can be ported 1:1.
// The host wires up the cButton pointers via
// SetButtonsForTest (replaces the legacy GetWindowForID
// lookups); the host calls SetActive when the player
// dies and the dialog is to be shown.

#pragma once

#include "cDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cButton;

class cReviveDialog : public cDialog {
public:
    cReviveDialog();
    ~cReviveDialog() override;

    cReviveDialog(const cReviveDialog&) = delete;
    cReviveDialog& operator=(const cReviveDialog&) = delete;

    // 1:1 with legacy Linking.  Resolves the 3 cButton
    // children via the host-injected pointers.
    void Linking();

    // 1:1 with legacy SetActive override.  Calls
    // cDialog::SetActive(val) then toggles the
    // Present / Village button visibility based on the
    // siege-war map (callback-supplied map num + siege
    // war map num).  The login button is always shown.
    void SetActive(bool val) noexcept override;

    // 1:1 with legacy WindowIDEnum.h CR_PRESENTSPOT /
    // CR_LOGINSPOT / CR_TOWNSPOT.
    static constexpr std::int32_t kIdPresentBtn = 30;
    static constexpr std::int32_t kIdLoginBtn  = 31;
    static constexpr std::int32_t kIdVillageBtn = 32;

    // Test hooks -- inject the 3 cButton pointers
    // (replaces the legacy GetWindowForID lookups).
    void SetButtonsForTest(cButton* present, cButton* login, cButton* village) noexcept {
        m_pPresentBtn = present; m_pLoginBtn = login; m_pVillageBtn = village;
    }
    cButton* GetPresentBtnForTest() const noexcept { return m_pPresentBtn; }
    cButton* GetLoginBtnForTest()   const noexcept { return m_pLoginBtn; }
    cButton* GetVillageBtnForTest() const noexcept { return m_pVillageBtn; }

    // Test hook -- inject a "current map num" callback
    // (legacy MAP->GetMapNum()).
    using MapNumCallback = std::uint32_t(*)(void* user);
    void SetMapNumCallbackForTest(MapNumCallback cb, void* user) {
        m_mapNumCb = cb; m_mapNumUser = user;
    }

    // Test hook -- inject a "siege war map num" callback
    // (legacy SIEGEMGR->GetSiegeWarMapNum()).
    void SetSiegeWarMapNumCallbackForTest(MapNumCallback cb, void* user) {
        m_siegeWarMapNumCb = cb; m_siegeWarMapNumUser = user;
    }

private:
    cButton*      m_pPresentBtn  = nullptr;
    cButton*      m_pLoginBtn    = nullptr;
    cButton*      m_pVillageBtn  = nullptr;
    MapNumCallback m_mapNumCb       = nullptr;
    void*         m_mapNumUser      = nullptr;
    MapNumCallback m_siegeWarMapNumCb = nullptr;
    void*         m_siegeWarMapNumUser = nullptr;
};

} // namespace mxh::ui
