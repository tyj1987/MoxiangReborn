// cExitDialog.cpp — modern port of 墨香 CExitDialog (exit confirmation).
#include "cExitDialog.hpp"

namespace mxh::ui {

void cExitDialog::Init(std::int32_t x, std::int32_t y, std::uint16_t wid,
                       std::uint16_t hei, void* basicImage, std::int32_t id) {
    cDialog::Init(x, y, wid, hei, basicImage, id);
    m_bExitActive = false;
    // The base cDialog::Init() leaves m_bActive=false; we keep
    // m_bExitActive in lock-step so isActive() and exitActive() agree
    // until the host explicitly calls SetActive().
}

void cExitDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CExitDialog::SetActive — propagate to base so the
    // cWindow visibility / dispatch chain sees the new state, and notify
    // the main-bar host via the callback (only on actual transitions).
    const bool wasActive = m_bExitActive;
    cDialog::SetActive(val);
    m_bExitActive = val;

    if (wasActive != val && m_onActiveChanged) {
        m_onActiveChanged(val);
    }
}

} // namespace mxh::ui
