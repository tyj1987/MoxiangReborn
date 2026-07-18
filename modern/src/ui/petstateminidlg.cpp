// petstateminidlg.cpp — modern port of 墨香 CPetStateMiniDlg (pet state UI).
//
// 1:1 port body. See legacy `PetStateMiniDlg.cpp` for the original.

#include "petstateminidlg.hpp"

#include "cButton.hpp"
#include "cGuagen.hpp"
#include "cStatic.hpp"
#include "cWindow.hpp"  // for WindowEvent enum

#include <cstdint>
#include <memory>

namespace mxh::ui {

namespace {

// 1:1 stub: legacy PETMGR->TogglePetStateDlg() / SendPetRestMsg() /
// OpenPetInvenDlg() are global singleton calls. Modern port has no
// PETMGR — host app would wire a real one. We provide a no-op stub
// for the 3 calls. To inject a real PETMGR, host app can define a
// real implementation of these functions and link it before the
// dialog lib (per Phase 6 stub pattern).
inline void StubTogglePetStateDlg()    { /* no-op */ }
inline void StubSendPetRestMsg(bool)   { /* no-op */ }
inline void StubOpenPetInvenDlg()      { /* no-op */ }
inline void* StubGetCurSummonPet()     { return nullptr; }  // no pet summoned

}  // namespace

cPetStateMiniDlg::cPetStateMiniDlg() = default;
cPetStateMiniDlg::~cPetStateMiniDlg() = default;

void cPetStateMiniDlg::Linking() {
    // 1:1 with legacy `Linking()` body — resolve 9 child windows by
    // id. Modern port materializes them as unique_ptr members; the
    // pattern matches cSkillPointNotify / cMoneyDlg.
    if (!m_pNameDlg) {
        auto p = std::make_unique<cStatic>();
        p->Init(0, 0, 80, 16, nullptr, kIdName);
        m_pNameDlg = std::move(p);
    }
    if (!m_pStateDlg) {
        auto p = std::make_unique<cStatic>();
        p->Init(0, 0, 80, 16, nullptr, kIdState);
        m_pStateDlg = std::move(p);
    }
    if (!m_pFriend) {
        auto p = std::make_unique<cStatic>();
        p->Init(0, 0, 60, 16, nullptr, kIdFriend);
        m_pFriend = std::move(p);
    }
    if (!m_pStamina) {
        auto p = std::make_unique<cStatic>();
        p->Init(0, 0, 60, 16, nullptr, kIdStamina);
        m_pStamina = std::move(p);
    }
    if (!m_pFriendGuage) {
        auto p = std::make_unique<cGuagen>();
        p->Init(0, 0, 60, 8, nullptr, kIdFriendGuage);
        m_pFriendGuage = std::move(p);
    }
    if (!m_pStaminaGuage) {
        auto p = std::make_unique<cGuagen>();
        p->Init(0, 0, 60, 8, nullptr, kIdStaminaGuage);
        m_pStaminaGuage = std::move(p);
    }
    if (!m_pPetUseRestBtn) {
        auto p = std::make_unique<cButton>();
        p->Init(0, 0, 16, 16, nullptr, nullptr, nullptr, nullptr, nullptr,
                kIdUseRestBtn);
        m_pPetUseRestBtn = std::move(p);
    }
    if (!m_pPetInvenBtn) {
        auto p = std::make_unique<cButton>();
        p->Init(0, 0, 16, 16, nullptr, nullptr, nullptr, nullptr, nullptr,
                kIdInvenBtn);
        m_pPetInvenBtn = std::move(p);
    }
    if (!m_pDlgToggleBtn) {
        auto p = std::make_unique<cButton>();
        p->Init(0, 0, 16, 16, nullptr, nullptr, nullptr, nullptr, nullptr,
                kIdToggleBtn);
        m_pDlgToggleBtn = std::move(p);
    }
}

void cPetStateMiniDlg::OnActionEvent(std::int32_t lId, void* /*p*/,
                                     std::uint32_t we) {
    // 1:1 with legacy: only WE_BTNCLICK branch is meaningful; all 3
    // button ids route to PETMGR global singleton calls. PETMGR is
    // stubbed; host app wires a real one.
    // 1:1 quirk: legacy `we & WE_BTNCLICK` where WE_BTNCLICK = 64 in
    // legacy enum. Modern WindowEvent::LButtonClick = 4 (a different
    // bit position). Modern port uses the modern cButton's return
    // value (LButtonClick) and checks via static_cast to WindowEvent.
    if (we != static_cast<std::uint32_t>(WindowEvent::LButtonClick)) {
        return;
    }
    if (lId == kIdToggleBtn) {
        StubTogglePetStateDlg();
    } else if (lId == kIdUseRestBtn) {
        if (StubGetCurSummonPet() == nullptr) {
            return;  // 1:1 quirk: no pet summoned → no-op
        }
        StubSendPetRestMsg(false);  // legacy: !IsPetRest() — pet is never rest in stub
    } else if (lId == kIdInvenBtn) {
        StubOpenPetInvenDlg();
    }
    // 1:1 quirk: legacy silently ignores unknown lId (no `else`
    // branch, no log). Modern port matches.
}

} // namespace mxh::ui
