#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct PetStateRefreshState { std::uint32_t pet_id{}; std::uint32_t hp{}; std::uint32_t max_hp{}; };
class cPetStateDlg final : public cDialog {
public:
 using PetStateRefreshCallback = std::function<bool(const PetStateRefreshState&)>;
 bool Set(PetStateRefreshState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetPetStateRefreshCallback(PetStateRefreshCallback cb) noexcept { m_petstaterefresh_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_petstaterefresh_cb && !m_petstaterefresh_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<PetStateRefreshState>& State() const noexcept { return m_state; }
private:
 std::optional<PetStateRefreshState> m_state{};
 bool m_confirmed{};
 PetStateRefreshCallback m_petstaterefresh_cb{};
};
}
