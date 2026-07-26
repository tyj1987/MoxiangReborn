#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct ServerSelectState { std::int32_t selected_index{}; std::uint16_t server_count{}; };
class cServerListDialog final : public cDialog {
public:
 using ServerSelectCallback = std::function<bool(const ServerSelectState&)>;
 bool Set(ServerSelectState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetServerSelectCallback(ServerSelectCallback cb) noexcept { m_serverselect_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_serverselect_cb && !m_serverselect_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<ServerSelectState>& State() const noexcept { return m_state; }
private:
 std::optional<ServerSelectState> m_state{};
 bool m_confirmed{};
 ServerSelectCallback m_serverselect_cb{};
};
}
