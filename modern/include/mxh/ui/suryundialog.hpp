#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct SuryunRequestState { std::uint32_t mugong_idx{}; std::uint8_t suryun_kind{}; };
class cSuryunDialog final : public cDialog {
public:
 using SuryunRequestCallback = std::function<bool(const SuryunRequestState&)>;
 bool Set(SuryunRequestState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetSuryunRequestCallback(SuryunRequestCallback cb) noexcept { m_suryunrequest_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_suryunrequest_cb && !m_suryunrequest_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<SuryunRequestState>& State() const noexcept { return m_state; }
private:
 std::optional<SuryunRequestState> m_state{};
 bool m_confirmed{};
 SuryunRequestCallback m_suryunrequest_cb{};
};
}
