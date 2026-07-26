#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct PetRevivalRequestState { std::uint32_t pet_id{}; std::uint32_t revival_cost{}; };
class cPetRevivalDialog final : public cDialog {
public:
 using PetRevivalRequestCallback = std::function<bool(const PetRevivalRequestState&)>;
 bool Set(PetRevivalRequestState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetPetRevivalRequestCallback(PetRevivalRequestCallback cb) noexcept { m_petrevivalrequest_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_petrevivalrequest_cb && !m_petrevivalrequest_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<PetRevivalRequestState>& State() const noexcept { return m_state; }
private:
 std::optional<PetRevivalRequestState> m_state{};
 bool m_confirmed{};
 PetRevivalRequestCallback m_petrevivalrequest_cb{};
};
}
