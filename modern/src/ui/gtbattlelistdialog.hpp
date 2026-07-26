#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct GTBattleListRefreshState { std::uint16_t current_round{}; std::uint16_t participants{}; };
class cGTBattleListDialog final : public cDialog {
public:
 using GTBattleListRefreshCallback = std::function<bool(const GTBattleListRefreshState&)>;
 bool Set(GTBattleListRefreshState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetGTBattleListRefreshCallback(GTBattleListRefreshCallback cb) noexcept { m_gtbattlelistrefresh_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_gtbattlelistrefresh_cb && !m_gtbattlelistrefresh_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<GTBattleListRefreshState>& State() const noexcept { return m_state; }
private:
 std::optional<GTBattleListRefreshState> m_state{};
 bool m_confirmed{};
 GTBattleListRefreshCallback m_gtbattlelistrefresh_cb{};
};
}
