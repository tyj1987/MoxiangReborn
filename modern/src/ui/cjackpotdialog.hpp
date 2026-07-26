#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct JackpotJoinState { std::uint32_t round_id{}; std::uint8_t participated{}; };
class ccJackpotDialog final : public cDialog {
public:
 using JackpotJoinCallback = std::function<bool(const JackpotJoinState&)>;
 bool Set(JackpotJoinState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetJackpotJoinCallback(JackpotJoinCallback cb) noexcept { m_jackpotjoin_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_jackpotjoin_cb && !m_jackpotjoin_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<JackpotJoinState>& State() const noexcept { return m_state; }
private:
 std::optional<JackpotJoinState> m_state{};
 bool m_confirmed{};
 JackpotJoinCallback m_jackpotjoin_cb{};
};
}
