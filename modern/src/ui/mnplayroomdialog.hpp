#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct MNPlayRoomTickState { std::uint32_t room_id{}; std::uint16_t team_red_score{}; std::uint16_t team_blue_score{}; };
class cMNPlayRoomDialog final : public cDialog {
public:
 using MNPlayRoomTickCallback = std::function<bool(const MNPlayRoomTickState&)>;
 bool Set(MNPlayRoomTickState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetMNPlayRoomTickCallback(MNPlayRoomTickCallback cb) noexcept { m_mnplayroomtick_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_mnplayroomtick_cb && !m_mnplayroomtick_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<MNPlayRoomTickState>& State() const noexcept { return m_state; }
private:
 std::optional<MNPlayRoomTickState> m_state{};
 bool m_confirmed{};
 MNPlayRoomTickCallback m_mnplayroomtick_cb{};
};
}
