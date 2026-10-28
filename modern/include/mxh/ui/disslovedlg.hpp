#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct DissloveRequestState { std::uint32_t item_id{}; std::uint16_t quantity{}; };
class cDissloveDlg final : public cDialog {
public:
 using DissloveRequestCallback = std::function<bool(const DissloveRequestState&)>;
 bool Set(DissloveRequestState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetDissloveRequestCallback(DissloveRequestCallback cb) noexcept { m_dissloverequest_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_dissloverequest_cb && !m_dissloverequest_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<DissloveRequestState>& State() const noexcept { return m_state; }
private:
 std::optional<DissloveRequestState> m_state{};
 bool m_confirmed{};
 DissloveRequestCallback m_dissloverequest_cb{};
};
}
