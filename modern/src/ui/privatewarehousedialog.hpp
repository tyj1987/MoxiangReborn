#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct PrivateWarehouseState { std::uint32_t warehouse_id{}; std::uint16_t slot_count{}; };
class cPrivateWarehouseDialog final : public cDialog {
public:
 using PrivateWarehouseCallback = std::function<bool(const PrivateWarehouseState&)>;
 bool Set(PrivateWarehouseState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetPrivateWarehouseCallback(PrivateWarehouseCallback cb) noexcept { m_privatewarehouse_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_privatewarehouse_cb && !m_privatewarehouse_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<PrivateWarehouseState>& State() const noexcept { return m_state; }
private:
 std::optional<PrivateWarehouseState> m_state{};
 bool m_confirmed{};
 PrivateWarehouseCallback m_privatewarehouse_cb{};
};
}
