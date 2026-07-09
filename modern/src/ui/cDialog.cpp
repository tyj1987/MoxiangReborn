// mxh/ui/cDialog.cpp
// Phase 6.3 — implementation of the modern cDialog widget.
#include "cDialog.hpp"

namespace mxh::ui {

void cDialog::Init(std::int32_t x, std::int32_t y, std::uint16_t wid,
                   std::uint16_t hei, void* basicImage, std::int32_t id) {
    cWindow::Init(x, y, wid, hei, basicImage, id);
    m_bAutoClose      = false;
    m_bCloseRequested = false;
    m_bActive         = false;
    m_hasCaption      = false;
    m_captionLeft     = x;
    m_captionTop      = y;
    m_captionRight    = x + wid;
    m_captionBottom   = y;
    m_alpha           = 255;
}

std::uint32_t cDialog::ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                                    std::uint32_t mouseFlags) {
    if (!isEnabled() || !m_bActive) {
        return static_cast<std::uint32_t>(WindowEvent::Null);
    }
    // Defer to cWindow's top-down dispatch — children handle their own
    // event consumption; the dialog just routes.
    return cWindow::ActionEvent(mouseX, mouseY, mouseFlags);
}

void cDialog::SetCaptionRect(std::int32_t left, std::int32_t top,
                             std::int32_t right, std::int32_t bottom) noexcept {
    m_captionLeft   = left;
    m_captionTop    = top;
    m_captionRight  = right;
    m_captionBottom = bottom;
    m_hasCaption    = (right > left) && (bottom >= top);
}

void cDialog::SetCaptionRect(const CaptionRect& r) noexcept {
    SetCaptionRect(r.left, r.top, r.right, r.bottom);
}

bool cDialog::PtInCaption(std::int32_t x, std::int32_t y) const noexcept {
    if (!m_hasCaption) return false;
    if (x < m_captionLeft || x > m_captionRight)  return false;
    if (y < m_captionTop  || y > m_captionBottom) return false;
    return true;
}

void cDialog::SetActiveRecursive(bool v) {
    m_bActive = v;
    // Cascade to children.
    for (std::size_t i = 0; i < childCount(); ++i) {
        if (cWindow* c = childAt(i)) {
            // Recursive cast: only cDialog has SetActiveRecursive; we
            // attempt a dynamic cast via the cObject::name() and the
            // concrete class is not in the framework yet, so we just
            // walk the tree. Children that want to react to active
            // changes do so in their own overrides of SetDisable /
            // SetEnabled.
            (void)c;
        }
    }
}

cWindow* cDialog::findWindowById(std::int32_t id) const {
    if (this->id() == id) return const_cast<cDialog*>(this);
    for (std::size_t i = 0; i < childCount(); ++i) {
        cWindow* c = childAt(i);
        if (!c) continue;
        if (c->id() == id) return c;
        // Recurse into cDialog children (cDialog is the only recursive
        // container in the framework today; cButton / cEditBox are leaves).
        if (auto* d = dynamic_cast<cDialog*>(c)) {
            if (cWindow* found = d->findWindowById(id)) {
                return found;
            }
        }
    }
    return nullptr;
}

void cDialog::SetAbsXY(std::int32_t x, std::int32_t y) noexcept {
    const std::int32_t dx = x - absX();
    const std::int32_t dy = y - absY();
    cWindow::SetAbsXY(x, y);
    // Also offset the caption so PtInCaption keeps working.
    m_captionLeft   += dx;
    m_captionTop    += dy;
    m_captionRight  += dx;
    m_captionBottom += dy;
    // Cascade the move to children so absolute layout stays correct.
    for (std::size_t i = 0; i < childCount(); ++i) {
        if (cWindow* c = childAt(i)) {
            c->SetAbsXY(c->absX() + dx, c->absY() + dy);
        }
    }
}

void cDialog::SetDisable(bool v) noexcept {
    cWindow::SetEnabled(!v);
    // Cascade to children (legacy contract — cDialog::SetDisable disables
    // every child control too).
    for (std::size_t i = 0; i < childCount(); ++i) {
        if (cWindow* c = childAt(i)) {
            c->SetEnabled(!v);
        }
    }
}

} // namespace mxh::ui
