#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct TitanInventoryState { std::uint32_t titan_id{}; std::uint16_t slot_count{}; };
class cTitanInventoryDlg final : public cDialog {
public:
 using TitanInventoryCallback = std::function<bool(const TitanInventoryState&)>;
 bool Set(TitanInventoryState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetTitanInventoryCallback(TitanInventoryCallback cb) noexcept { m_titaninventory_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_titaninventory_cb && !m_titaninventory_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<TitanInventoryState>& State() const noexcept { return m_state; }
private:
 std::optional<TitanInventoryState> m_state{};
 bool m_confirmed{};
 TitanInventoryCallback m_titaninventory_cb{};
};
}
