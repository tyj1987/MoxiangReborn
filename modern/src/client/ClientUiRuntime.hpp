#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cDialogLoader.hpp"
#include "mxh/ui/cMsgBox.hpp"
#include "mxh/ui/cWindowManager.hpp"

namespace mxh::ui {
class cWindow;
}

namespace mxh::client {

struct ClientUiActivation {
    std::int32_t numeric_id = 0;
    std::string legacy_id;
    std::string legacy_func;
    std::string dialog_legacy_id;
};

struct ClientUiInputResult {
    bool consumed = false;
    std::optional<ClientUiActivation> activation;
};

// State-scoped owner and input dispatcher for one InterfaceScript tree.
// Network callbacks never touch this object; the host invokes it exclusively
// from the client main thread.
class ClientUiRuntime {
public:
    ClientUiRuntime() = default;
    ~ClientUiRuntime();

    ClientUiRuntime(const ClientUiRuntime&) = delete;
    ClientUiRuntime& operator=(const ClientUiRuntime&) = delete;

    bool load(const std::filesystem::path& playdh_root,
              std::string_view script_name,
              mxh::ui::ResolutionMode mode,
              std::string* error = nullptr);
    bool loadMany(const std::filesystem::path& playdh_root,
                  std::span<const std::string_view> script_names,
                  mxh::ui::ResolutionMode mode,
                  std::string* error = nullptr);
    void clear() noexcept;
    void abandon_for_process_exit() noexcept;

    void setActive(bool active) noexcept;
    bool isActive() const noexcept { return m_active; }
    bool empty() const noexcept { return m_windows.dialogCount() == 0; }
    mxh::ui::cWindow* focusedWindow() const noexcept { return m_focused; }

    ClientUiInputResult onMouseButton(bool left, bool down,
                                     std::int32_t x, std::int32_t y);
    bool onMouseMove(std::int32_t x, std::int32_t y);
    bool onMouseWheel(std::int32_t wheelDelta) noexcept;
    bool onKey(bool down, std::int32_t key);
    bool onKey(bool down, std::int32_t key, bool shift);
    std::optional<ClientUiActivation> consumeKeyActivation() noexcept {
        auto activation = std::move(m_keyActivation);
        m_keyActivation.reset();
        return activation;
    }
    bool onChar(std::int32_t ch);

    bool setDialogActive(std::string_view legacy_id, bool active) noexcept;
    bool isDialogActive(std::string_view legacy_id) const noexcept;
    // Update the manager's active layout mode after a committed display
    // transition.  Existing controls keep their logical 4:3 coordinates;
    // the mode change ensures subsequent dialog loads and hit-testing use
    // the same resolution policy as the renderer.
    void onResolutionChange(mxh::ui::ResolutionMode mode) noexcept;
    mxh::ui::ResolutionMode resolution_mode() const noexcept {
        return m_windows.currentResolutionMode();
    }
    // CharSelect / CharMake load a single script whose roots must be
    // visible and hittable. GameIn must not call this — inventory/shop
    // stay inactive until I/B/Q.
    void activateAllLoadedDialogs() noexcept;
    // Activate only the listed legacy ids; every other loaded dialog is
    // hidden so it cannot swallow world clicks.
    void applyActiveSet(std::span<const std::string_view> active_ids) noexcept;

    using ConfirmationCallback = std::function<void(bool confirmed)>;
    bool showConfirmation(std::int32_t id, std::string message,
                          ConfirmationCallback callback);
    using MessageCallback = std::function<void()>;
    bool showMessage(std::int32_t id, std::string message,
                     MessageCallback callback = {});
    bool hasModal() const noexcept { return m_windows.isModal(); }

    void render();

    // Update all progress gauges in the active state UI. Returns the number
    // of gauge controls updated; loading states use this instead of drawing
    // a debug-only percentage overlay.
    std::size_t setProgressValue(float value) noexcept;

    mxh::ui::cWindow* findWindowByLegacyId(std::string_view id) const;
    bool focusWindowByLegacyId(std::string_view id) noexcept;
    mxh::ui::cWindow* findWindowByLegacyFunc(std::string_view func) const;
    const std::vector<std::unique_ptr<mxh::ui::cDialog>>& dialogs() const noexcept {
        return m_windows.dialogs();
    }
    std::vector<std::unique_ptr<mxh::ui::cDialog>>& dialogsMutable() noexcept {
        return m_windows.dialogs();
    }

private:
    mxh::ui::cWindow* hitTest(std::int32_t x, std::int32_t y) const noexcept;
    void focus(mxh::ui::cWindow* window) noexcept;
    void focusNext() noexcept;
    void focusPrevious() noexcept;
    void collectClosedModal() noexcept;
    mxh::ui::cMsgBox* createMessageBox(std::int32_t id,
                                      std::string message,
                                      mxh::ui::cMsgBox::MBType type,
                                      mxh::ui::cMsgBox::MsgBoxCallback callback);

    mxh::ui::cWindowManager m_windows;
    mxh::ui::cWindow* m_focused = nullptr;
    mxh::ui::cWindow* m_pressedLeft = nullptr;
    bool m_active = false;
    std::optional<ClientUiActivation> m_keyActivation;
};

} // namespace mxh::client
