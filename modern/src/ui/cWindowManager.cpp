// mxh/ui/cWindowManager.cpp
// Phase 6.6 — implementation of the modern cWindowManager.
#include "mxh/ui/resolution_mode.hpp"  // M-R7 (G3) — must come first for full type
#include "cWindowManager.hpp"

#include "cButton.hpp"
#include "cDialog.hpp"
#include "cEditBox.hpp"

namespace mxh::ui {

cWindowManager::~cWindowManager() {
    // Clear focus BEFORE destroying dialogs. m_focused is a raw pointer
    // into the dialog tree; once the unique_ptrs in m_dialogs are
    // released the pointer would dangle. SetFocus(nullptr) routes
    // through our null-safe branch.
    if (m_focused) {
        m_focused->SetFocus(false);
        m_focused = nullptr;
    }
    m_modalDialog = nullptr;  // raw pointer, not owned — just drop
    m_dialogs.clear();
    m_destroyQueue.clear();
}

void cWindowManager::AddDialog(std::unique_ptr<cDialog> dlg) {
    if (!dlg) return;
    m_dialogs.push_back(std::move(dlg));
}

std::unique_ptr<cDialog> cWindowManager::RemoveDialog(cDialog* dlg) {
    if (!dlg) return {};
    for (auto it = m_dialogs.begin(); it != m_dialogs.end(); ++it) {
        if (it->get() == dlg) {
            std::unique_ptr<cDialog> taken = std::move(*it);
            m_dialogs.erase(it);
            // Clear modal pointer if we just removed the modal dialog.
            if (m_modalDialog == dlg) m_modalDialog = nullptr;
            return taken;
        }
    }
    return {};
}

bool cWindowManager::RemoveDialogById(std::int32_t id) {
    cDialog* d = findById(id);
    if (!d) return false;
    auto taken = RemoveDialog(d);
    if (taken) {
        // Defer destruction: the caller may be iterating dialogs.
        m_destroyQueue.push_back(std::move(taken));
    }
    return true;
}

void cWindowManager::RemoveAll() {
    // Move everything to the destroy queue (defer-destroy semantics).
    for (auto& d : m_dialogs) {
        if (d) m_destroyQueue.push_back(std::move(d));
    }
    m_dialogs.clear();
    m_modalDialog = nullptr;
}

void cWindowManager::ProcessDestroyQueue() {
    m_destroyQueue.clear();
}

cDialog* cWindowManager::topmost() const noexcept {
    if (m_dialogs.empty()) return nullptr;
    return m_dialogs.back().get();
}

cDialog* cWindowManager::topmostActive() const noexcept {
    // Walk back-to-front; first active wins.
    for (auto it = m_dialogs.rbegin(); it != m_dialogs.rend(); ++it) {
        cDialog* d = it->get();
        if (d && d->isActive()) return d;
    }
    return nullptr;
}

cDialog* cWindowManager::findById(std::int32_t id) const {
    for (const auto& d : m_dialogs) {
        if (!d) continue;
        if (d->id() == id) return d.get();
        cWindow* w = d->findWindowById(id);
        if (!w) continue;
        // The match might be the dialog itself or a child; we want
        // the topmost owning dialog.
        if (w == d.get()) return d.get();
        cObject* p = w->parent();
        while (p && p->parent()) p = p->parent();
        return d.get();
    }
    return nullptr;
}

cWindow* cWindowManager::findWindowByLegacyId(std::string_view id) const {
    // Skip inactive dialogs: the legacy engine only routes input to the
    // active top-level dialog, so lookups must mirror that. Otherwise a
    // hidden dialog sharing the same legacy id (CMI_CLOSEBTN lives in
    // many .bin files) shadows the one the player can actually see.
    for (auto it = m_dialogs.rbegin(); it != m_dialogs.rend(); ++it) {
        if (!*it || !(*it)->isActive()) continue;
        if (cWindow* found = (*it)->findWindowByLegacyId(id)) {
            return found;
        }
    }
    return nullptr;
}

cWindow* cWindowManager::findWindowByLegacyFunc(std::string_view func) const {
    // See findWindowByLegacyId: the active dialog owns input first.
    for (auto it = m_dialogs.rbegin(); it != m_dialogs.rend(); ++it) {
        if (!*it || !(*it)->isActive()) continue;
        if (cWindow* found = (*it)->findWindowByLegacyFunc(func)) {
            return found;
        }
    }
    return nullptr;
}

cDialog* cWindowManager::findByXY(std::int32_t x, std::int32_t y) const {
    // Walk back-to-front; the first dialog that contains (x,y) wins.
    for (auto it = m_dialogs.rbegin(); it != m_dialogs.rend(); ++it) {
        cDialog* d = it->get();
        if (!d) continue;
        if (d->PtInWindow(x, y)) return d;
    }
    return nullptr;
}

void cWindowManager::SetModalDialog(cDialog* dlg) noexcept {
    // Marking a dialog as modal also flips its "active" flag so its
    // children (e.g. cMsgBox buttons) can fire their callbacks via the
    // cDialog::ActionEvent fast path. This matches the legacy contract
    // where SetModal called SetActive(TRUE) under the hood.
    if (m_modalDialog && m_modalDialog != dlg) {
        m_modalDialog->SetActive(false);
    }
    m_modalDialog = dlg;
    if (m_modalDialog) m_modalDialog->SetActive(true);
}

std::uint32_t cWindowManager::ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                                          std::uint32_t mouseFlags) {
    cDialog* target = m_modalDialog ? m_modalDialog : topmostActive();
    if (!target) return static_cast<std::uint32_t>(cWindow::WindowEvent::Null);
    return target->ActionEvent(mouseX, mouseY, mouseFlags);
}

std::uint32_t cWindowManager::ActionKeyboardEvent(std::int32_t key,
                                                   std::int32_t ch) {
    // Keyboard input goes to the focused dialog. We pick the topmost
    // active dialog as the receiver; production code would track focus
    // explicitly, but for the skeleton this matches the legacy
    // engine's "the active dialog owns the keyboard" contract.
    cDialog* target = m_modalDialog ? m_modalDialog : topmostActive();
    if (!target) return static_cast<std::uint32_t>(cWindow::WindowEvent::Null);
    return target->ActionKeyboardEvent(key, ch);
}

void cWindowManager::RenderAll() {
    // Walk back-to-front so the topmost active dialog draws last.  Legacy
    // #ACTIVE 0 roots remain owned by the manager but are neither rendered
    // nor eligible for input until gameplay explicitly opens them.
    for (const auto& d : m_dialogs) {
        if (d && d->isActive() && d->isVisible()) d->Render();
    }
}

// M-R7 (G3) 分辨率自适应 1:1 行为.
void cWindowManager::SetCurrentResolutionMode(ResolutionMode mode) noexcept {
    m_resolutionMode = mode;
}

void cWindowManager::OnResolutionChange(ResolutionMode new_mode) noexcept {
    // 1:1 老版 cScriptManager 没这个 API. modern 暴露是为了 caller (MoxianClient main)
    // 切换屏幕尺寸时通知 cWindowManager. 实际重新装载由 caller 决定
    // (cWindowManager 不持 .bin 路径). 头less 头实现是更新 m_resolutionMode.
    m_resolutionMode = new_mode;
}

// ===========================================================================
// M-R6.2 Focus chain — 1:1 with legacy cWindowManager::SetFocus /
// TabFocusNext / TabFocusPrev.
//
// Legacy semantics:
//   - One focused window pointer across the whole dialog tree.
//   - Tab cycles to the next focusable child in the topmost active
//     dialog's z-order (children added with cDialog::Add are stored
//     back-to-front; we treat back as z=0 + front as z=N-1 for
//     purposes of "next").
//   - Shift+Tab reverses.
//   - If no candidate exists, focus stays put (no wrap).
//   - SetFocus on the same window is a no-op (no re-fire).
// ===========================================================================

namespace {

// True if `w` is a Tab-focus target. 1:1 with legacy cWindow::IsFocusable:
//   cEditBox  -> focusable
//   cButton   -> focusable (cPushupButton subclass also focusable)
//   cStatic / cGuageBar / cGuagen  -> not focusable
bool is_focusable_class(const cWindow* w) {
    if (!w) return false;
    if (dynamic_cast<const cEditBox*>(w)) return true;
    if (dynamic_cast<const cButton*>(w)) return true;
    return false;
}

}  // namespace

bool cWindowManager::isFocusableCandidate(const cWindow* w) noexcept {
    return is_focusable_class(w);
}

cWindow* cWindowManager::findFocusableAfter(cDialog* dlg, std::int32_t fromIdx) const {
    if (!dlg) return nullptr;
    const std::size_t n = dlg->childCount();
    for (std::int32_t i = fromIdx + 1; i < static_cast<std::int32_t>(n); ++i) {
        cWindow* kid = dlg->childAt(static_cast<std::size_t>(i));
        if (is_focusable_class(kid)) return kid;
    }
    return nullptr;
}

cWindow* cWindowManager::findFocusableBefore(cDialog* dlg, std::int32_t fromIdx) const {
    if (!dlg) return nullptr;
    for (std::int32_t i = fromIdx - 1; i >= 0; --i) {
        cWindow* kid = dlg->childAt(static_cast<std::size_t>(i));
        if (is_focusable_class(kid)) return kid;
    }
    return nullptr;
}

void cWindowManager::SetFocus(cWindow* w) {
    if (w == m_focused) return;  // no-op (1:1 legacy)
    if (m_focused) m_focused->SetFocus(false);
    m_focused = w;
    if (m_focused) m_focused->SetFocus(true);
}

void cWindowManager::TabFocusNext() {
    cDialog* top = topmostActive();
    if (!top) return;
    std::int32_t idx = -1;
    if (m_focused && m_focused->parent() == top) {
        const std::size_t n = top->childCount();
        for (std::size_t i = 0; i < n; ++i) {
            if (top->childAt(i) == m_focused) {
                idx = static_cast<std::int32_t>(i);
                break;
            }
        }
    }
    cWindow* next = findFocusableAfter(top, idx);
    if (next) SetFocus(next);
    // else: no candidate — focus stays put (1:1 with legacy)
}

void cWindowManager::TabFocusPrev() {
    cDialog* top = topmostActive();
    if (!top) return;
    std::int32_t idx = static_cast<std::int32_t>(top->childCount());
    if (m_focused && m_focused->parent() == top) {
        const std::size_t n = top->childCount();
        for (std::size_t i = 0; i < n; ++i) {
            if (top->childAt(i) == m_focused) {
                idx = static_cast<std::int32_t>(i);
                break;
            }
        }
    }
    cWindow* prev = findFocusableBefore(top, idx);
    if (prev) SetFocus(prev);
}

std::int32_t cWindowManager::focusedId() const noexcept {
    if (!m_focused) return 0;
    return m_focused->id();
}

} // namespace mxh::ui
