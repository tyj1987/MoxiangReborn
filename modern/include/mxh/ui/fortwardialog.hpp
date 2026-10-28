#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct FortWarRequestState { std::uint8_t attack_team_id{}; std::uint8_t defend_team_id{}; std::uint32_t fort_id{}; };
class cFortWarDialog final : public cDialog {
public:
 using FortWarRequestCallback = std::function<bool(const FortWarRequestState&)>;
 bool Set(FortWarRequestState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetFortWarRequestCallback(FortWarRequestCallback cb) noexcept { m_fortwarrequest_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_fortwarrequest_cb && !m_fortwarrequest_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<FortWarRequestState>& State() const noexcept { return m_state; }
private:
 std::optional<FortWarRequestState> m_state{};
 bool m_confirmed{};
 FortWarRequestCallback m_fortwarrequest_cb{};
};
}
