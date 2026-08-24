#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct ReinforceResetState { std::uint32_t target_item{}; std::uint8_t reset_tier{}; };
class cReinforceResetDlg final : public cDialog {
public:
 using ReinforceResetCallback = std::function<bool(const ReinforceResetState&)>;
 bool Set(ReinforceResetState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetReinforceResetCallback(ReinforceResetCallback cb) noexcept { m_reinforcereset_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_reinforcereset_cb && !m_reinforcereset_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<ReinforceResetState>& State() const noexcept { return m_state; }
private:
 std::optional<ReinforceResetState> m_state{};
 bool m_confirmed{};
 ReinforceResetCallback m_reinforcereset_cb{};
};
}
