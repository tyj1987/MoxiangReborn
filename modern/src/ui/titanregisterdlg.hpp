#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct TitanRegisterState { std::uint32_t titan_id{}; };
class cTitanRegisterDlg final : public cDialog {
public:
 using TitanRegisterCallback = std::function<bool(const TitanRegisterState&)>;
 bool Set(TitanRegisterState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetTitanRegisterCallback(TitanRegisterCallback cb) noexcept { m_titanregister_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_titanregister_cb && !m_titanregister_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<TitanRegisterState>& State() const noexcept { return m_state; }
private:
 std::optional<TitanRegisterState> m_state{};
 bool m_confirmed{};
 TitanRegisterCallback m_titanregister_cb{};
};
}
