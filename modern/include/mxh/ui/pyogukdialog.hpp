#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct WarehouseState { std::uint32_t warehouse_id{}; std::uint16_t slot_count{}; };
class cPyoGukDialog final : public cDialog {
public:
 using WarehouseCallback = std::function<bool(const WarehouseState&)>;
 bool Set(WarehouseState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetWarehouseCallback(WarehouseCallback cb) noexcept { m_warehouse_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_warehouse_cb && !m_warehouse_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<WarehouseState>& State() const noexcept { return m_state; }
private:
 std::optional<WarehouseState> m_state{};
 bool m_confirmed{};
 WarehouseCallback m_warehouse_cb{};
};
}
