#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct GTScoreInfoRefreshState { std::uint8_t team_id{}; std::uint32_t ranking{}; };
class cGTScoreInfoDialog final : public cDialog {
public:
 using GTScoreInfoRefreshCallback = std::function<bool(const GTScoreInfoRefreshState&)>;
 bool Set(GTScoreInfoRefreshState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetGTScoreInfoRefreshCallback(GTScoreInfoRefreshCallback cb) noexcept { m_gtscoreinforefresh_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_gtscoreinforefresh_cb && !m_gtscoreinforefresh_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<GTScoreInfoRefreshState>& State() const noexcept { return m_state; }
private:
 std::optional<GTScoreInfoRefreshState> m_state{};
 bool m_confirmed{};
 GTScoreInfoRefreshCallback m_gtscoreinforefresh_cb{};
};
}
