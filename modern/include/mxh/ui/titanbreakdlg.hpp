#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct TitanBreakState { std::uint32_t titan_id{}; std::uint16_t item_count{}; };
class cTitanBreakDlg final : public cDialog {
public:
 using TitanBreakCallback = std::function<bool(const TitanBreakState&)>;
 bool Set(TitanBreakState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetTitanBreakCallback(TitanBreakCallback cb) noexcept { m_titanbreak_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_titanbreak_cb && !m_titanbreak_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<TitanBreakState>& State() const noexcept { return m_state; }
private:
 std::optional<TitanBreakState> m_state{};
 bool m_confirmed{};
 TitanBreakCallback m_titanbreak_cb{};
};
}
