#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct PetInventoryRefreshState { std::uint32_t pet_id{}; std::uint16_t slot_count{}; };
class cPetInventoryDlg final : public cDialog {
public:
 using PetInventoryRefreshCallback = std::function<bool(const PetInventoryRefreshState&)>;
 bool Set(PetInventoryRefreshState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetPetInventoryRefreshCallback(PetInventoryRefreshCallback cb) noexcept { m_petinventoryrefresh_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_petinventoryrefresh_cb && !m_petinventoryrefresh_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<PetInventoryRefreshState>& State() const noexcept { return m_state; }
private:
 std::optional<PetInventoryRefreshState> m_state{};
 bool m_confirmed{};
 PetInventoryRefreshCallback m_petinventoryrefresh_cb{};
};
}
