// crevivedialog.cpp -- modern implementation of Moxiang CReviveDialog.

#include "crevivedialog.hpp"

#include "cbutton.hpp"

namespace mxh::ui {

cReviveDialog::cReviveDialog() = default;

cReviveDialog::~cReviveDialog() = default;

void cReviveDialog::Linking() {
    // 1:1 with legacy CReviveDialog::Linking.
    //   m_pPresentBtn  = (cButton*)GetWindowForID(CR_PRESENTSPOT);
    //   m_pLoginBtn    = (cButton*)GetWindowForID(CR_LOGINSPOT);
    //   m_pVillageBtn  = (cButton*)GetWindowForID(CR_TOWNSPOT);
    // The modern port lets the host inject the cButton
    // pointers via SetButtonsForTest (called before
    // Linking).
}

void cReviveDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CReviveDialog::SetActive override.
    cDialog::SetActive(val);
    // 1:1 siege-war branch:
    //   if( SIEGEMGR->GetSiegeWarMapNum() &&
    //       MAP->GetMapNum() == SIEGEMGR->GetSiegeWarMapNum() )
    //     m_pPresentBtn->SetActive(FALSE);
    //     m_pVillageBtn->SetActive(TRUE);
    //   else
    //     m_pPresentBtn->SetActive(TRUE);
    //     m_pVillageBtn->SetActive(FALSE);
    const std::uint32_t currentMap = m_mapNumCb ? m_mapNumCb(m_mapNumUser) : 0u;
    const std::uint32_t siegeMap   = m_siegeWarMapNumCb ? m_siegeWarMapNumCb(m_siegeWarMapNumUser) : 0u;
    const bool inSiege = (siegeMap != 0u) && (currentMap == siegeMap);
    if (m_pPresentBtn) m_pPresentBtn->SetActive(!inSiege);
    if (m_pVillageBtn) m_pVillageBtn->SetActive(inSiege);
}

} // namespace mxh::ui
