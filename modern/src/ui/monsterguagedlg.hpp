#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct MonsterGuageRefreshState { std::uint32_t target_id{}; std::uint32_t hp{}; std::uint32_t max_hp{}; };
class cMonsterGuageDlg final : public cDialog {
public:
 using MonsterGuageRefreshCallback = std::function<bool(const MonsterGuageRefreshState&)>;
 bool Set(MonsterGuageRefreshState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetMonsterGuageRefreshCallback(MonsterGuageRefreshCallback cb) noexcept { m_monsterguagerefresh_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_monsterguagerefresh_cb && !m_monsterguagerefresh_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<MonsterGuageRefreshState>& State() const noexcept { return m_state; }
private:
 std::optional<MonsterGuageRefreshState> m_state{};
 bool m_confirmed{};
 MonsterGuageRefreshCallback m_monsterguagerefresh_cb{};
};
}
