#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct UpgradeRequestState { std::uint32_t item_id{}; std::uint8_t target_tier{}; };
class cUpgradeDlg final : public cDialog {
public:
 using UpgradeRequestCallback = std::function<bool(const UpgradeRequestState&)>;
 bool Set(UpgradeRequestState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetUpgradeRequestCallback(UpgradeRequestCallback cb) noexcept { m_upgraderequest_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_upgraderequest_cb && !m_upgraderequest_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<UpgradeRequestState>& State() const noexcept { return m_state; }
private:
 std::optional<UpgradeRequestState> m_state{};
 bool m_confirmed{};
 UpgradeRequestCallback m_upgraderequest_cb{};
};
}
