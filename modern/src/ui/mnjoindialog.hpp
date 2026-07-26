#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct MNJoinRoomState { std::uint32_t target_room_id{}; std::uint32_t password_input{}; };
class cMNJoinDialog final : public cDialog {
public:
 using MNJoinRoomCallback = std::function<bool(const MNJoinRoomState&)>;
 bool Set(MNJoinRoomState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetMNJoinRoomCallback(MNJoinRoomCallback cb) noexcept { m_mnjoinroom_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_mnjoinroom_cb && !m_mnjoinroom_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<MNJoinRoomState>& State() const noexcept { return m_state; }
private:
 std::optional<MNJoinRoomState> m_state{};
 bool m_confirmed{};
 MNJoinRoomCallback m_mnjoinroom_cb{};
};
}
