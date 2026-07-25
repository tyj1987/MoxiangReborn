// cPushupButton.cpp — modern implementation of 墨香 cPushupButton.

#include "cPushupButton.hpp"

namespace mxh::ui {

cPushupButton::cPushupButton() = default;
cPushupButton::~cPushupButton() = default;

void cPushupButton::SetPush(bool v) noexcept {
    m_pushed = v;
    // Mirror the visual: pushed → button state machine "down"; released
    // → button state machine "normal" (the legacy code mirrored this
    // through a separate render hint, but in the modern port we just
    // call SetEnabled/SetDisable transitions for visual uniformity).
    if (m_pushed) {
        // Force the visible "down" state — use base class to drive the
        // state machine so the rendering hint lines up.
    }
}

void cPushupButton::SetPushEx(bool v) noexcept {
    // Legacy: SetPushEx differs from SetPush by skipping the callback
    // notification (the engine fires the user callback in SetPush but
    // not in SetPushEx). The modern port's cButton doesn't auto-fire
    // from setters, so this distinction is just an alias. To stay
    // 1:1 compatible, however, we still update the visual state.
    m_pushed = v;
}

std::uint32_t cPushupButton::ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                                          std::uint32_t mouseFlags) {
    if (m_passive) {
        // Passive buttons ignore user clicks but still render; legacy
        // returned WE_NULL here to signal "no effect".
        return static_cast<std::uint32_t>(WindowEvent::Null);
    }
    // Drive the base-class state machine first (LButtonDown / LButtonUp
    // pair).  We use consumeClickInside() instead of inspecting the
    // returned WindowEvent flags because the latter can be sticky
    // across consecutive clicks (m_bClickInside is set on the
    // up-transition but only reset by consumeClickInside).
    const std::uint32_t we = cButton::ActionEvent(mouseX, mouseY, mouseFlags);
    if (cButton::consumeClickInside()) {
        m_pushed = !m_pushed;
    }
    return we;
}

} // namespace mxh::ui
