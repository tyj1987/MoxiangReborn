#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct MNCreateRoomState { std::uint8_t room_name{}; std::uint8_t max_players{}; std::uint32_t password_hash{}; };
class cMNCreateDialog final : public cDialog {
public:
 using MNCreateRoomCallback = std::function<bool(const MNCreateRoomState&)>;
 bool Set(MNCreateRoomState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetMNCreateRoomCallback(MNCreateRoomCallback cb) noexcept { m_mncreateroom_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_mncreateroom_cb && !m_mncreateroom_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<MNCreateRoomState>& State() const noexcept { return m_state; }
private:
 std::optional<MNCreateRoomState> m_state{};
 bool m_confirmed{};
 MNCreateRoomCallback m_mncreateroom_cb{};
};
}
