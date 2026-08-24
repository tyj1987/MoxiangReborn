#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct BuyRegState { std::uint32_t item_id{}; std::uint16_t quantity{}; };
class cBuyRegDialog final : public cDialog {
public:
 using BuyRegCallback = std::function<bool(const BuyRegState&)>;
 bool Set(BuyRegState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetBuyRegCallback(BuyRegCallback cb) noexcept { m_buyreg_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_buyreg_cb && !m_buyreg_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<BuyRegState>& State() const noexcept { return m_state; }
private:
 std::optional<BuyRegState> m_state{};
 bool m_confirmed{};
 BuyRegCallback m_buyreg_cb{};
};
}
