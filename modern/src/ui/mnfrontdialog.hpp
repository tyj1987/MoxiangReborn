#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct MNFrontRefreshState { std::uint8_t current_page{}; std::uint8_t total_pages{}; };
class cMNFrontDialog final : public cDialog {
public:
 using MNFrontRefreshCallback = std::function<bool(const MNFrontRefreshState&)>;
 bool Set(MNFrontRefreshState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetMNFrontRefreshCallback(MNFrontRefreshCallback cb) noexcept { m_mnfrontrefresh_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_mnfrontrefresh_cb && !m_mnfrontrefresh_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<MNFrontRefreshState>& State() const noexcept { return m_state; }
private:
 std::optional<MNFrontRefreshState> m_state{};
 bool m_confirmed{};
 MNFrontRefreshCallback m_mnfrontrefresh_cb{};
};
}
