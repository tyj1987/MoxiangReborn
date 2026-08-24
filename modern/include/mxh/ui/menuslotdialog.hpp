#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct MenuSlotAssignState { std::uint8_t slot_index{}; };
class cMenuSlotDialog final : public cDialog {
public:
 using MenuSlotAssignCallback = std::function<bool(const MenuSlotAssignState&)>;
 bool Set(MenuSlotAssignState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetMenuSlotAssignCallback(MenuSlotAssignCallback cb) noexcept { m_menuslotassign_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_menuslotassign_cb && !m_menuslotassign_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<MenuSlotAssignState>& State() const noexcept { return m_state; }
private:
 std::optional<MenuSlotAssignState> m_state{};
 bool m_confirmed{};
 MenuSlotAssignCallback m_menuslotassign_cb{};
};
}
