#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct PetUpgradeRequestState { std::uint32_t pet_id{}; std::uint8_t target_level{}; };
class cPetUpgradeDialog final : public cDialog {
public:
 using PetUpgradeRequestCallback = std::function<bool(const PetUpgradeRequestState&)>;
 bool Set(PetUpgradeRequestState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetPetUpgradeRequestCallback(PetUpgradeRequestCallback cb) noexcept { m_petupgraderequest_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_petupgraderequest_cb && !m_petupgraderequest_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<PetUpgradeRequestState>& State() const noexcept { return m_state; }
private:
 std::optional<PetUpgradeRequestState> m_state{};
 bool m_confirmed{};
 PetUpgradeRequestCallback m_petupgraderequest_cb{};
};
}
