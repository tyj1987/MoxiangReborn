#include "ClientUiRuntime.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <vector>

#include "mxh/ui/cButton.hpp"
#include "mxh/ui/ccheckbox.hpp"
#include "mxh/ui/ccombobox.hpp"
#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cEditBox.hpp"
#include "mxh/ui/cGuagen.hpp"
#include "mxh/ui/cListCtrl.hpp"
#include "mxh/ui/cListDialog.hpp"
#include "mxh/ui/cMsgBox.hpp"
#include "mxh/ui/cWindow.hpp"

namespace mxh::client {
namespace {

bool shift_key_down() noexcept {
#ifdef _WIN32
    return (GetKeyState(VK_SHIFT) & 0x8000) != 0;
#else
    return false;
#endif
}

bool is_focusable_control(const mxh::ui::cWindow* window) noexcept {
    return dynamic_cast<const mxh::ui::cEditBox*>(window) ||
           dynamic_cast<const mxh::ui::cButton*>(window) ||
           dynamic_cast<const mxh::ui::cCheckBox*>(window) ||
           dynamic_cast<const mxh::ui::cComboBox*>(window) ||
           dynamic_cast<const mxh::ui::cListCtrl*>(window) ||
           dynamic_cast<const mxh::ui::cListDialog*>(window);
}

mxh::ui::cWindow* hit_window(mxh::ui::cWindow* window,
                             std::int32_t x, std::int32_t y) noexcept {
    if (!window || !window->isVisible() || !window->isEnabled()) {
        return nullptr;
    }
    // Original cDialog::ActionEventComponent walks children without
    // requiring the parent rect to contain the point. IDDlg.bin's
    // #POINT height is the caption (50) while OK/ID sit at y=150.
    for (std::size_t i = window->childCount(); i > 0; --i) {
        if (auto* hit = hit_window(window->childAt(i - 1), x, y)) return hit;
    }
    if (!window->PtInWindow(x, y)) return nullptr;
    return window;
}

void collect_focusable(mxh::ui::cWindow* window,
                       std::vector<mxh::ui::cWindow*>& out) {
    if (!window || !window->isVisible() || !window->isEnabled()) return;
    if (is_focusable_control(window)) {
        out.push_back(window);
    }
    for (std::size_t i = 0; i < window->childCount(); ++i) {
        collect_focusable(window->childAt(i), out);
    }
}

ClientUiActivation activation_for(const mxh::ui::cWindow& window) {
    const mxh::ui::cObject* root = &window;
    while (root->parent()) root = root->parent();
    const auto* dialog = dynamic_cast<const mxh::ui::cDialog*>(root);
    return ClientUiActivation{
        window.id(), window.legacyId(), window.legacyFunc(),
        dialog ? dialog->legacyId() : std::string{}};
}

} // namespace

ClientUiRuntime::~ClientUiRuntime() {
    clear();
}

bool ClientUiRuntime::load(const std::filesystem::path& playdh_root,
                           std::string_view script_name,
                           mxh::ui::ResolutionMode mode,
                           std::string* error) {
    const std::string_view scripts[] = {script_name};
    return loadMany(playdh_root, scripts, mode, error);
}

bool ClientUiRuntime::loadMany(
    const std::filesystem::path& playdh_root,
    std::span<const std::string_view> script_names,
    mxh::ui::ResolutionMode mode,
    std::string* error) {
    if (script_names.empty()) {
        if (error) *error = "No InterfaceScript names supplied";
        return false;
    }

    // Stage the complete set first.  A missing or malformed required script
    // must not replace the state's currently-valid UI with a partial tree.
    mxh::ui::cWindowManager staged;
    staged.SetCurrentResolutionMode(mode);
    for (const auto script_name : script_names) {
        const auto before = staged.dialogCount();
        const auto path = playdh_root / "Image" / "InterfaceScript" /
                          std::filesystem::path(script_name);
        const auto report = mxh::ui::cDialogLoader::LoadOne(path, staged, mode);
        if (!report.ok || staged.dialogCount() == before) {
            if (error) {
                *error = std::string(script_name) + ": " +
                    (report.error.empty()
                        ? "InterfaceScript produced no dialog roots"
                        : report.error);
            }
            return false;
        }
    }

    clear();
    while (staged.dialogCount() != 0) {
        auto* first = staged.dialogs().front().get();
        m_windows.AddDialog(staged.RemoveDialog(first));
    }
    m_windows.SetCurrentResolutionMode(mode);
    setActive(true);
    if (error) error->clear();
    return true;
}

void ClientUiRuntime::clear() noexcept {
    setActive(false);
    m_windows.SetFocus(nullptr);
    m_windows.RemoveAll();
    m_windows.ProcessDestroyQueue();
}

void ClientUiRuntime::abandon_for_process_exit() noexcept {
    m_active = false;
    m_focused = nullptr;
    m_pressedLeft = nullptr;
    m_windows.AbandonAllForProcessExit();
}

void ClientUiRuntime::setActive(bool active) noexcept {
    m_active = active;
    if (!active) {
        focus(nullptr);
        m_pressedLeft = nullptr;
    }
}

bool ClientUiRuntime::setDialogActive(std::string_view legacy_id,
                                      bool active) noexcept {
    for (const auto& dialog : m_windows.dialogs()) {
        if (dialog && dialog->legacyId() == legacy_id) {
            // Legacy contract: an inactive dialog is hidden from the
            // renderer and from hit-testing. The cDialog base class
            // only flips m_bActive; mirror that into visibility so the
            // dispatcher (which gates on m_bVisible) sees the change.
            dialog->SetActive(active);
            dialog->SetVisible(active);
            if (!active && m_focused) {
                const mxh::ui::cObject* root = m_focused;
                while (root->parent()) root = root->parent();
                if (root == dialog.get()) focus(nullptr);
            }
            return true;
        }
    }
    // InterfaceScript tab sheets and embedded panels are not top-level
    // dialogs.  They still carry their own legacy IDs and must participate
    // in the same visibility/hit-test contract when a client state switches
    // tabs at runtime.
    for (const auto& dialog : m_windows.dialogs()) {
        if (!dialog) continue;
        if (auto* window = dialog->findWindowByLegacyId(legacy_id)) {
            window->SetActive(active);
            window->SetVisible(active);
            return true;
        }
    }
    return false;
}

bool ClientUiRuntime::isDialogActive(std::string_view legacy_id) const noexcept {
    // This is an inspection API, not an input-routing lookup: inactive tab
    // sheets must remain discoverable so callers can verify their state.
    for (const auto& dialog : m_windows.dialogs()) {
        if (!dialog) continue;
        if (dialog->legacyId() == legacy_id) {
            return dialog->isActive();
        }
        if (const auto* window = dialog->findWindowByLegacyId(legacy_id)) {
            return window->isActive();
        }
    }
    return false;
}

void ClientUiRuntime::activateAllLoadedDialogs() noexcept {
    m_active = true;
    for (const auto& dialog : m_windows.dialogs()) {
        if (!dialog) continue;
        dialog->SetActive(true);
        dialog->SetVisible(true);
    }
}

void ClientUiRuntime::applyActiveSet(
    std::span<const std::string_view> active_ids) noexcept {
    m_active = true;
    for (const auto& dialog : m_windows.dialogs()) {
        if (!dialog) continue;
        // Reset both the root and any previously selected page before
        // applying the new set; otherwise a tab that was active in the
        // previous screen can remain visible after a state transition.
        dialog->SetActive(false);
        dialog->SetVisible(false);
        for (const auto id : active_ids) {
            if (dialog->legacyId() == id) {
                dialog->SetActive(true);
                dialog->SetVisible(true);
                break;
            }
            // A child page/tab is still rendered through its parent dialog;
            // hiding the root when only the child ID is active makes the
            // otherwise valid tab appear to disappear entirely.
            if (auto* child = dialog->findWindowByLegacyId(id)) {
                dialog->SetActive(true);
                dialog->SetVisible(true);
                child->SetActive(true);
                child->SetVisible(true);
                break;
            }
        }
    }
    bool focus_valid = m_focused != nullptr;
    for (auto* node = static_cast<mxh::ui::cObject*>(m_focused);
         node && focus_valid; node = node->parent()) {
        const auto* window = dynamic_cast<const mxh::ui::cWindow*>(node);
        if (!window || !window->isVisible() || !window->isEnabled()) {
            focus_valid = false;
        }
    }
    if (!focus_valid) {
        focus(nullptr);
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

void ClientUiRuntime::focusPrevious() noexcept {
    std::vector<mxh::ui::cWindow*> focusable;
    for (const auto& dialog : m_windows.dialogs()) {
        collect_focusable(dialog.get(), focusable);
    }
    if (focusable.empty()) {
        focus(nullptr);
        return;
    }
    const auto current = std::find(focusable.begin(), focusable.end(), m_focused);
    if (current == focusable.end() || current == focusable.begin()) {
        focus(focusable.back());
        return;
    }
    focus(*std::prev(current));
}

ClientUiInputResult ClientUiRuntime::onMouseButton(
    bool left, bool down, std::int32_t x, std::int32_t y) {
    ClientUiInputResult result;
    if (!m_active) return result;

    if (m_windows.isModal()) {
        const auto flags = left && down ? mxh::ui::cWindow::MouseFlagLButton
            : (!left && down ? mxh::ui::cWindow::MouseFlagRButton : 0u);
        m_windows.ActionEvent(x, y, flags);
        result.consumed = true;
        collectClosedModal();
        return result;
    }

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
        if (is_focusable_control(hit)) focus(hit);
        else if (m_focused != hit) focus(nullptr);
        hit->ActionEvent(x, y, mxh::ui::cWindow::MouseFlagLButton);
        result.consumed = true;
        return result;
    }

    mxh::ui::cWindow* pressed = m_pressedLeft;
    m_pressedLeft = nullptr;
    if (!pressed) return result;
    (void)hit; (void)pressed;
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
    if (m_windows.isModal()) {
        m_windows.ActionEvent(x, y, 0u);
        collectClosedModal();
        return true;
    }
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

bool ClientUiRuntime::onMouseWheel(std::int32_t wheelDelta) noexcept {
    if (!m_active || !m_focused || wheelDelta == 0) return false;
    if (auto* list = dynamic_cast<mxh::ui::cListCtrl*>(m_focused)) {
        list->OnMouseWheel(wheelDelta);
        return true;
    }
    if (auto* list = dynamic_cast<mxh::ui::cListDialog*>(m_focused)) {
        list->OnMouseWheel(wheelDelta);
        return true;
    }
    return false;
}

bool ClientUiRuntime::onKey(bool down, std::int32_t key) {
    return onKey(down, key, shift_key_down());
}

bool ClientUiRuntime::onKey(bool down, std::int32_t key, bool shift) {
    if (!m_active || !down) return false;
    if (m_windows.isModal()) {
        m_windows.ActionKeyboardEvent(key, 0);
        collectClosedModal();
        return true;
    }
    if (key == 9) {
        if (shift) focusPrevious();
        else focusNext();
        return true;
    }
    if (!m_focused) return false;
    if (key == 13 || key == 32) {
        if (auto* button = dynamic_cast<mxh::ui::cButton*>(m_focused)) {
            const auto x = button->absX() + button->width() / 2;
            const auto y = button->absY() + button->height() / 2;
            (void)onMouseButton(true, true, x, y);
            const auto result = onMouseButton(true, false, x, y);
            if (result.activation) m_keyActivation = *result.activation;
            return true;
        }
    }
    return m_focused->ActionKeyboardEvent(key, 0) !=
           static_cast<std::uint32_t>(mxh::ui::cWindow::WindowEvent::Null);
}

bool ClientUiRuntime::onChar(std::int32_t ch) {
    if (m_active && m_windows.isModal()) return true;
    if (!m_active || !m_focused) return false;
    return m_focused->ActionKeyboardEvent(0, ch) !=
           static_cast<std::uint32_t>(mxh::ui::cWindow::WindowEvent::Null);
}

void ClientUiRuntime::render() {
    if (m_active) m_windows.RenderAll();
}

std::size_t ClientUiRuntime::setProgressValue(float value) noexcept {
    value = std::clamp(value, 0.0f, 1.0f);
    std::size_t updated = 0;
    const auto visit = [&](auto&& self, mxh::ui::cWindow* window) -> void {
        if (!window) return;
        if (auto* gauge = dynamic_cast<mxh::ui::cGuagen*>(window)) {
            gauge->SetValue(value);
            ++updated;
        }
        for (std::size_t i = 0; i < window->childCount(); ++i) {
            self(self, window->childAt(i));
        }
    };
    for (auto& dialog : m_windows.dialogs()) visit(visit, dialog.get());
    return updated;
}

bool ClientUiRuntime::showConfirmation(std::int32_t id, std::string message,
                                       ConfirmationCallback callback) {
    if (!m_active || m_windows.isModal()) return false;
    return createMessageBox(id, std::move(message),
        mxh::ui::cMsgBox::MBType::YesNo,
        [callback = std::move(callback)](mxh::ui::cMsgBox&,
                                         mxh::ui::cMsgBox::MBResult result,
                                         void*) {
            if (callback) callback(result == mxh::ui::cMsgBox::MBResult::Yes);
        }) != nullptr;
}

bool ClientUiRuntime::showMessage(std::int32_t id, std::string message,
                                  MessageCallback callback) {
    if (!m_active || m_windows.isModal()) return false;
    return createMessageBox(id, std::move(message), mxh::ui::cMsgBox::MBType::Ok,
        [callback = std::move(callback)](mxh::ui::cMsgBox&,
                                         mxh::ui::cMsgBox::MBResult,
                                         void*) {
            if (callback) callback();
        }) != nullptr;
}

mxh::ui::cMsgBox* ClientUiRuntime::createMessageBox(
    std::int32_t id, std::string message, mxh::ui::cMsgBox::MBType type,
    mxh::ui::cMsgBox::MsgBoxCallback callback) {
    constexpr std::int32_t width = 197;
    constexpr std::int32_t height = 150;
    const auto [screen_width, screen_height] = mxh::ui::mode_size(
        m_windows.currentResolutionMode());
    auto box = std::make_unique<mxh::ui::cMsgBox>();
    box->Init((screen_width - width) / 2, (screen_height - height) / 2,
              static_cast<std::uint16_t>(width),
              static_cast<std::uint16_t>(height),
              mxh::ui::cDialogLoader::LoadLegacyImage(30), id);
    box->SetButtonImages(
        mxh::ui::cDialogLoader::LoadLegacyImage(31),
        mxh::ui::cDialogLoader::LoadLegacyImage(32),
        mxh::ui::cDialogLoader::LoadLegacyImage(33));
    box->MsgBox(id, type, message, std::move(callback));
    auto* modal = box.get();
    focus(nullptr);
    m_pressedLeft = nullptr;
    m_windows.AddDialog(std::move(box));
    m_windows.SetModalDialog(modal);
    return modal;
}

void ClientUiRuntime::collectClosedModal() noexcept {
    auto* modal = m_windows.modalDialog();
    if (!modal || !modal->closeRequested()) return;
    focus(nullptr);
    m_pressedLeft = nullptr;
    auto retired = m_windows.RemoveDialog(modal);
    retired.reset();
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
