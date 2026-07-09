// mxh/ui/cButton.cpp
// Phase 6.1 — implementation of the modern cButton widget.
#include "cButton.hpp"

namespace mxh::ui {

void cButton::Init(std::int32_t x, std::int32_t y, std::uint16_t wid,
                   std::uint16_t hei, void* basicImage, void* overImage,
                   void* pressImage, ClickCallback onClick, void* userdata,
                   std::int32_t id) {
    cWindow::Init(x, y, wid, hei, basicImage, id);
    m_basicImage = basicImage;
    m_overImage  = overImage;
    m_pressImage = pressImage;
    m_onClick    = std::move(onClick);
    m_userdata   = userdata;
    m_state = isEnabled() ? State::Normal : State::Disabled;
    m_bClickInside = false;
    recomputeCurrentTextColor();
}

void cButton::SetText(std::string text, ColorRGBA basic, ColorRGBA over,
                      ColorRGBA press) {
    m_text         = std::move(text);
    m_fgBasicColor = basic;
    // 0 means "use basic" for over/press — this matches the legacy
    // cButton::SetText default args (over=NULL, Press=NULL).
    m_fgOverColor  = (over  != 0) ? over  : basic;
    m_fgPressColor = (press != 0) ? press : basic;
    recomputeCurrentTextColor();
}

void cButton::SetTextXY(std::int32_t x, std::int32_t y) noexcept {
    m_textX = x;
    m_textY = y;
}

void cButton::SetShadowTextXY(std::int32_t x, std::int32_t y) noexcept {
    m_shadowTextX = x;
    m_shadowTextY = y;
}

void cButton::SetEnabled(bool v) noexcept {
    cWindow::SetEnabled(v);
    m_state = v ? State::Normal : State::Disabled;
    if (!v) m_bClickInside = false;
    recomputeCurrentTextColor();
}

bool cButton::consumeClickInside() noexcept {
    const bool was = m_bClickInside;
    m_bClickInside = false;
    return was;
}

void cButton::recomputeCurrentTextColor() {
    switch (m_state) {
        case State::Hover:   m_fgCurColor = m_fgOverColor;  break;
        case State::Pressed: m_fgCurColor = m_fgPressColor; break;
        case State::Disabled:
        case State::Normal:
        default:            m_fgCurColor = m_fgBasicColor; break;
    }
}

void cButton::transitionTo(State newState) {
    if (m_state == newState) return;
    m_state = newState;
    recomputeCurrentTextColor();
}

std::uint32_t cButton::ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                                    std::uint32_t mouseFlags) {
    // Disabled buttons are dead to all events.
    if (!isEnabled()) {
        return static_cast<std::uint32_t>(WindowEvent::Null);
    }
    const bool inside = PtInWindow(mouseX, mouseY);
    const bool lbutton = (mouseFlags & MouseFlagLButton) != 0;

    // State machine.
    if (inside) {
        if (lbutton) {
            transitionTo(State::Pressed);
        } else {
            // If we were pressed and the user released inside, the click
            // fires here.
            if (m_state == State::Pressed) {
                m_bClickInside = true;
                if (m_onClick) {
                    m_onClick(id(), m_userdata);
                }
            }
            transitionTo(State::Hover);
        }
    } else {
        if (lbutton) {
            // Pressed but dragged out: cancel click, stay in Pressed until
            // released outside (then revert to Normal).
            if (m_state != State::Pressed) {
                // The user pressed outside then dragged in. Treat as a fresh
                // press on entering.
                transitionTo(State::Pressed);
            }
        } else {
            // LButtonUp outside. If we were pressed, the click is cancelled.
            // Otherwise, cursor simply left the area.
            if (m_state == State::Pressed) {
                // Drag-out cancel.
                transitionTo(State::Normal);
            } else {
                transitionTo(State::Normal);
            }
        }
    }

    // Surface a stable WE_* event for the engine's event log. The detailed
    // click is consumed via consumeClickInside() — the WE_* return here is
    // for the dispatcher / focus chain.
    if (m_bClickInside) {
        return static_cast<std::uint32_t>(WindowEvent::LButtonClick);
    }
    if (lbutton && inside) {
        return static_cast<std::uint32_t>(WindowEvent::LButtonDown);
    }
    return static_cast<std::uint32_t>(WindowEvent::Null);
}

} // namespace mxh::ui
