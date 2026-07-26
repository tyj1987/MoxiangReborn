#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct PartyMatchingState { std::uint32_t leader_id{}; std::uint8_t min_level{}; };
class cPartyMatchingDlg final : public cDialog {
public:
 using PartyMatchingCallback = std::function<bool(const PartyMatchingState&)>;
 bool Set(PartyMatchingState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetPartyMatchingCallback(PartyMatchingCallback cb) noexcept { m_partymatching_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_partymatching_cb && !m_partymatching_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<PartyMatchingState>& State() const noexcept { return m_state; }
private:
 std::optional<PartyMatchingState> m_state{};
 bool m_confirmed{};
 PartyMatchingCallback m_partymatching_cb{};
};
}
