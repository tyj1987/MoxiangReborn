#include "ClientUiRuntime.hpp"

#include <algorithm>
#include <vector>

#include "mxh/ui/cButton.hpp"
#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cEditBox.hpp"
#include "mxh/ui/cWindow.hpp"

namespace mxh::client {
namespace {

mxh::ui::cWindow* hit_window(mxh::ui::cWindow* window,
                             std::int32_t x, std::int32_t y) noexcept {
    if (!window || !window->isVisible() || !window->isEnabled() ||
        !window->PtInWindow(x, y)) {
        return nullptr;
    }
    for (std::size_t i = window->childCount(); i > 0; --i) {
        if (auto* hit = hit_window(window->childAt(i - 1), x, y)) return hit;
    }
    return window;
}

void collect_focusable(mxh::ui::cWindow* window,
                       std::vector<mxh::ui::cWindow*>& out) {
    if (!window || !window->isVisible() || !window->isEnabled()) return;
    if (dynamic_cast<mxh::ui::cEditBox*>(window) ||
        dynamic_cast<mxh::ui::cButton*>(window)) {
        out.push_back(window);
    }
    for (std::size_t i = 0; i < window->childCount(); ++i) {
        collect_focusable(window->childAt(i), out);
    }
}

ClientUiActivation activation_for(const mxh::ui::cWindow& window) {
    return ClientUiActivation{
        window.id(), window.legacyId(), window.legacyFunc()};
}

} // namespace

ClientUiRuntime::~ClientUiRuntime() {
    clear();
}

bool ClientUiRuntime::load(const std::filesystem::path& playdh_root,
                           std::string_view script_name,
                           mxh::ui::ResolutionMode mode,
                           std::string* error) {
    clear();
    const auto path = playdh_root / "Image" / "InterfaceScript" /
                      std::filesystem::path(script_name);
    const auto report = mxh::ui::cDialogLoader::LoadOne(path, m_windows, mode);
    if (!report.ok || m_windows.dialogCount() == 0) {
        if (error) {
            *error = report.error.empty()
                ? "InterfaceScript produced no dialog roots"
                : report.error;
        }
        clear();
        return false;
    }
    setActive(true);
    return true;
}

void ClientUiRuntime::clear() noexcept {
    setActive(false);
    m_windows.SetFocus(nullptr);
    m_windows.RemoveAll();
    m_windows.ProcessDestroyQueue();
}

void ClientUiRuntime::setActive(bool active) noexcept {
    m_active = active;
    if (!active) {
        focus(nullptr);
        m_pressedLeft = nullptr;
    }
    for (const auto& dialog : m_windows.dialogs()) {
        if (dialog) dialog->SetActive(active);
    }
}

mxh::ui::cWindow* ClientUiRuntime::hitTest(std::int32_t x,
                                           std::int32_t y) const noexcept {
    if (!m_active) return nullptr;
    const auto& dialogs = m_windows.dialogs();
    for (std::size_t i = dialogs.size(); i > 0; --i) {
        mxh::ui::cDialog* dialog = dialogs[i - 1].get();
        if (!dialog || !dialog->isActive()) continue;
        if (auto* hit = hit_window(dialog, x, y)) return hit;
    }
    return nullptr;
}

void ClientUiRuntime::focus(mxh::ui::cWindow* window) noexcept {
    if (m_focused == window) return;
    if (m_focused) m_focused->SetFocus(false);
    m_focused = window;
    m_windows.SetFocus(window);
}

void ClientUiRuntime::focusNext() noexcept {
    std::vector<mxh::ui::cWindow*> focusable;
    for (const auto& dialog : m_windows.dialogs()) {
        collect_focusable(dialog.get(), focusable);
    }
    if (focusable.empty()) {
        focus(nullptr);
        return;
    }
    const auto current = std::find(focusable.begin(), focusable.end(), m_focused);
    if (current == focusable.end()) {
        focus(focusable.front());
        return;
    }
    const auto next = std::next(current);
    focus(next == focusable.end() ? focusable.front() : *next);
}

ClientUiInputResult ClientUiRuntime::onMouseButton(
    bool left, bool down, std::int32_t x, std::int32_t y) {
    ClientUiInputResult result;
    if (!m_active) return result;

    mxh::ui::cWindow* hit = hitTest(x, y);
    if (!left) {
        if (!hit) return result;
        const auto flags = down ? mxh::ui::cWindow::MouseFlagRButton : 0u;
        hit->ActionEvent(x, y, flags);
        result.consumed = true;
        return result;
    }

    if (down) {
        m_pressedLeft = hit;
        if (!hit) {
            focus(nullptr);
            return result;
        }
        if (dynamic_cast<mxh::ui::cEditBox*>(hit)) focus(hit);
        else if (m_focused != hit) focus(nullptr);
        hit->ActionEvent(x, y, mxh::ui::cWindow::MouseFlagLButton);
        result.consumed = true;
        return result;
    }

    mxh::ui::cWindow* pressed = m_pressedLeft;
    m_pressedLeft = nullptr;
    if (!pressed) return result;
    const auto event = pressed->ActionEvent(x, y, 0u);
    result.consumed = true;
    if (pressed == hit) {
        if (auto* button = dynamic_cast<mxh::ui::cButton*>(pressed);
            button) {
            const bool clicked = button->consumeClickInside() ||
                event == static_cast<std::uint32_t>(
                    mxh::ui::cWindow::WindowEvent::LButtonClick);
            if (clicked) result.activation = activation_for(*button);
        }
    }
    return result;
}

bool ClientUiRuntime::onMouseMove(std::int32_t x, std::int32_t y) {
    if (!m_active) return false;
    if (m_pressedLeft) {
        m_pressedLeft->ActionEvent(
            x, y, mxh::ui::cWindow::MouseFlagLButton);
        return true;
    }
    if (auto* hit = hitTest(x, y)) {
        hit->ActionEvent(x, y, 0u);
        return true;
    }
    return false;
}

bool ClientUiRuntime::onKey(bool down, std::int32_t key) {
    if (!m_active || !down) return false;
    if (key == 9) {
        focusNext();
        return true;
    }
    if (!m_focused) return false;
    return m_focused->ActionKeyboardEvent(key, 0) !=
           static_cast<std::uint32_t>(mxh::ui::cWindow::WindowEvent::Null);
}

bool ClientUiRuntime::onChar(std::int32_t ch) {
    if (!m_active || !m_focused) return false;
    return m_focused->ActionKeyboardEvent(0, ch) !=
           static_cast<std::uint32_t>(mxh::ui::cWindow::WindowEvent::Null);
}

void ClientUiRuntime::render() {
    if (m_active) m_windows.RenderAll();
}

mxh::ui::cWindow* ClientUiRuntime::findWindowByLegacyId(
    std::string_view id) const {
    return m_windows.findWindowByLegacyId(id);
}

mxh::ui::cWindow* ClientUiRuntime::findWindowByLegacyFunc(
    std::string_view func) const {
    return m_windows.findWindowByLegacyFunc(func);
}

} // namespace mxh::client
