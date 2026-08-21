#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>

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

    void setActive(bool active) noexcept;
    bool isActive() const noexcept { return m_active; }
    bool empty() const noexcept { return m_windows.dialogCount() == 0; }

    ClientUiInputResult onMouseButton(bool left, bool down,
                                     std::int32_t x, std::int32_t y);
    bool onMouseMove(std::int32_t x, std::int32_t y);
    bool onKey(bool down, std::int32_t key);
    bool onChar(std::int32_t ch);

    bool setDialogActive(std::string_view legacy_id, bool active) noexcept;
    bool isDialogActive(std::string_view legacy_id) const noexcept;

    using ConfirmationCallback = std::function<void(bool confirmed)>;
    bool showConfirmation(std::int32_t id, std::string message,
                          ConfirmationCallback callback);
    using MessageCallback = std::function<void()>;
    bool showMessage(std::int32_t id, std::string message,
                     MessageCallback callback = {});
    bool hasModal() const noexcept { return m_windows.isModal(); }

    void render();

    mxh::ui::cWindow* findWindowByLegacyId(std::string_view id) const;
    mxh::ui::cWindow* findWindowByLegacyFunc(std::string_view func) const;
    const std::vector<std::unique_ptr<mxh::ui::cDialog>>& dialogs() const noexcept {
        return m_windows.dialogs();
    }

private:
    mxh::ui::cWindow* hitTest(std::int32_t x, std::int32_t y) const noexcept;
    void focus(mxh::ui::cWindow* window) noexcept;
    void focusNext() noexcept;
    void collectClosedModal() noexcept;
    mxh::ui::cMsgBox* createMessageBox(std::int32_t id,
                                      std::string message,
                                      mxh::ui::cMsgBox::MBType type,
                                      mxh::ui::cMsgBox::MsgBoxCallback callback);

    mxh::ui::cWindowManager m_windows;
    mxh::ui::cWindow* m_focused = nullptr;
    mxh::ui::cWindow* m_pressedLeft = nullptr;
    bool m_active = false;
};

} // namespace mxh::client
