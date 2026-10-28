#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct TitanPartsMakeState { std::uint32_t titan_id{}; std::uint8_t part_kind{}; };
class cTitanPartsMakeDlg final : public cDialog {
public:
 using TitanPartsMakeCallback = std::function<bool(const TitanPartsMakeState&)>;
 bool Set(TitanPartsMakeState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetTitanPartsMakeCallback(TitanPartsMakeCallback cb) noexcept { m_titanpartsmake_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_titanpartsmake_cb && !m_titanpartsmake_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<TitanPartsMakeState>& State() const noexcept { return m_state; }
private:
 std::optional<TitanPartsMakeState> m_state{};
 bool m_confirmed{};
 TitanPartsMakeCallback m_titanpartsmake_cb{};
};
}
