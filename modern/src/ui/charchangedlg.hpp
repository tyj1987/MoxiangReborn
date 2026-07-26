#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct CharSlotPickState { std::uint8_t current_slot{}; };
class cCharChangeDlg final : public cDialog {
public:
 using CharSlotPickCallback = std::function<bool(const CharSlotPickState&)>;
 bool Set(CharSlotPickState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetCharSlotPickCallback(CharSlotPickCallback cb) noexcept { m_charslotpick_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_charslotpick_cb && !m_charslotpick_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<CharSlotPickState>& State() const noexcept { return m_state; }
private:
 std::optional<CharSlotPickState> m_state{};
 bool m_confirmed{};
 CharSlotPickCallback m_charslotpick_cb{};
};
}
