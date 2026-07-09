// mxh/ui/cWindow.cpp
// Phase 6.0 — implementation of the modern cWindow framework skeleton.
// See cWindow.hpp for the contract; this file is kept separate from the
// header so consumers that include cWindow.hpp don't pay the cost of the
// method bodies in every translation unit.
#include "cWindow.hpp"

namespace mxh::ui {

void cWindow::Init(std::int32_t x, std::int32_t y, std::uint16_t wid,
                   std::uint16_t hei, void* basicImage, std::int32_t id) {
    m_absX = m_relX = m_validX = x;
    m_absY = m_relY = m_validY = y;
    m_w = wid;
    m_h = hei;
    m_basicImage = basicImage;
    mutableId() = id;
}

bool cWindow::PtInWindow(std::int32_t x, std::int32_t y) const noexcept {
    // Inclusive on the boundary: the legacy engine treats a click on the
    // window's right/bottom edge as "inside" so cursor edge-snap behaviors
    // (drag handles, scrollbar thumbs) work correctly.
    if (x < m_absX || x > m_absX + static_cast<std::int32_t>(m_w)) return false;
    if (y < m_absY || y > m_absY + static_cast<std::int32_t>(m_h)) return false;
    return true;
}

void cWindow::SetAbsXY(std::int32_t x, std::int32_t y) noexcept {
    m_absX = x;
    m_absY = y;
}

void cWindow::SetRelXY(std::int32_t x, std::int32_t y) noexcept {
    m_relX = x;
    m_relY = y;
}

void cWindow::SetValidXY(std::int32_t x, std::int32_t y) noexcept {
    m_validX = x;
    m_validY = y;
}

void cWindow::SetWH(std::int32_t w, std::int32_t h) noexcept {
    if (w < 0) w = 0;
    if (h < 0) h = 0;
    if (w > 0xFFFF) w = 0xFFFF;
    if (h > 0xFFFF) h = 0xFFFF;
    m_w = static_cast<std::uint16_t>(w);
    m_h = static_cast<std::uint16_t>(h);
}

void cWindow::Add(std::unique_ptr<cWindow> child) {
    if (!child) return;
    child->setParent(this);
    // Convert the child's absolute coords to be relative to *this* if the
    // caller has been using SetAbsXY; the legacy engine's default behavior
    // is "parent's abs + child's rel" = child's abs. We preserve the
    // child's absX/absY verbatim and just record the parent link — the
    // rendering layer is responsible for combining absX + parent.absX
    // when computing screen positions in 6.1.2.
    m_children.push_back(std::move(child));
}

cWindow* cWindow::childAt(std::size_t i) const noexcept {
    return i < m_children.size() ? m_children[i].get() : nullptr;
}

std::unique_ptr<cWindow> cWindow::removeChildAt(std::size_t i) {
    if (i >= m_children.size()) return {};
    auto up = std::move(m_children[i]);
    m_children.erase(m_children.begin() + i);
    if (up) up->setParent(nullptr);
    return up;
}

std::uint32_t cWindow::ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                                    std::uint32_t mouseFlags) {
    if (!m_bVisible || !m_bEnabled) {
        return static_cast<std::uint32_t>(WindowEvent::Null);
    }
    // Top-down dispatch: recurse into the topmost (last-added) child first.
    // If any child consumes the event, we're done. We use index-based reverse
    // iteration to dodge an MSVC ICE in <xutility> that triggers on
    // std::unique_ptr reverse-iterator range-for in C++20.
    for (std::size_t i = m_children.size(); i > 0; --i) {
        cWindow* c = m_children[i - 1].get();
        if (!c) continue;
        const std::uint32_t ev = c->ActionEvent(mouseX, mouseY, mouseFlags);
        if (ev != static_cast<std::uint32_t>(WindowEvent::Null)) {
            return ev;
        }
    }
    if (!PtInWindow(mouseX, mouseY)) {
        return static_cast<std::uint32_t>(WindowEvent::Null);
    }
    // We were hit. Map mouseFlags to a WE_* event.
    if (mouseFlags & MouseFlagLButton) {
        return static_cast<std::uint32_t>(WindowEvent::LButtonClick);
    }
    if (mouseFlags & MouseFlagRButton) {
        return static_cast<std::uint32_t>(WindowEvent::RButtonClick);
    }
    return static_cast<std::uint32_t>(WindowEvent::MouseMove);
}

} // namespace mxh::ui
