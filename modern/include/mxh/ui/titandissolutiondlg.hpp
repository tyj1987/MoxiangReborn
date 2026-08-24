#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct TitanDissolutionState { std::uint32_t titan_id{}; };
class cTitanDissolutionDlg final : public cDialog {
public:
 using TitanDissolutionCallback = std::function<bool(const TitanDissolutionState&)>;
 bool Set(TitanDissolutionState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetTitanDissolutionCallback(TitanDissolutionCallback cb) noexcept { m_titandissolution_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_titandissolution_cb && !m_titandissolution_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<TitanDissolutionState>& State() const noexcept { return m_state; }
private:
 std::optional<TitanDissolutionState> m_state{};
 bool m_confirmed{};
 TitanDissolutionCallback m_titandissolution_cb{};
};
}
