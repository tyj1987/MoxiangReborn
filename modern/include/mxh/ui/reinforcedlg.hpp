#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct ReinforceState { std::uint32_t target_item{}; std::uint8_t tier{}; std::uint32_t cost_gold{}; };
class cReinforceDlg final : public cDialog {
public:
 using ReinforceCallback = std::function<bool(const ReinforceState&)>;
 bool Set(ReinforceState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetReinforceCallback(ReinforceCallback cb) noexcept { m_reinforce_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_reinforce_cb && !m_reinforce_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<ReinforceState>& State() const noexcept { return m_state; }
private:
 std::optional<ReinforceState> m_state{};
 bool m_confirmed{};
 ReinforceCallback m_reinforce_cb{};
};
}
