#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct GTStandingRefreshState { std::uint16_t season_id{}; std::uint16_t rank_index{}; };
class cGTStandingDialog final : public cDialog {
public:
 using GTStandingRefreshCallback = std::function<bool(const GTStandingRefreshState&)>;
 bool Set(GTStandingRefreshState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetGTStandingRefreshCallback(GTStandingRefreshCallback cb) noexcept { m_gtstandingrefresh_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_gtstandingrefresh_cb && !m_gtstandingrefresh_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<GTStandingRefreshState>& State() const noexcept { return m_state; }
private:
 std::optional<GTStandingRefreshState> m_state{};
 bool m_confirmed{};
 GTStandingRefreshCallback m_gtstandingrefresh_cb{};
};
}
