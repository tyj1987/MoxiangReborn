#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct StreetStallItemViewState { std::uint32_t stall_id{}; std::uint16_t item_count{}; };
class cStreetStallItemViewDlg final : public cDialog {
public:
 using StreetStallItemViewCallback = std::function<bool(const StreetStallItemViewState&)>;
 bool Set(StreetStallItemViewState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetStreetStallItemViewCallback(StreetStallItemViewCallback cb) noexcept { m_streetstallitemview_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_streetstallitemview_cb && !m_streetstallitemview_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<StreetStallItemViewState>& State() const noexcept { return m_state; }
private:
 std::optional<StreetStallItemViewState> m_state{};
 bool m_confirmed{};
 StreetStallItemViewCallback m_streetstallitemview_cb{};
};
}
