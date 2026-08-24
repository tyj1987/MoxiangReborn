#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct TitanUpgradeState { std::uint32_t titan_id{}; std::uint8_t target_level{}; };
class cTitanUpgradeDlg final : public cDialog {
public:
 using TitanUpgradeCallback = std::function<bool(const TitanUpgradeState&)>;
 bool Set(TitanUpgradeState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetTitanUpgradeCallback(TitanUpgradeCallback cb) noexcept { m_titanupgrade_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_titanupgrade_cb && !m_titanupgrade_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<TitanUpgradeState>& State() const noexcept { return m_state; }
private:
 std::optional<TitanUpgradeState> m_state{};
 bool m_confirmed{};
 TitanUpgradeCallback m_titanupgrade_cb{};
};
}
