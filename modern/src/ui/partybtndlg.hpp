#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct PartyBtnStateState { std::uint32_t party_id{}; std::uint8_t active{}; };
class cPartyBtnDlg final : public cDialog {
public:
 using PartyBtnStateCallback = std::function<bool(const PartyBtnStateState&)>;
 bool Set(PartyBtnStateState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetPartyBtnStateCallback(PartyBtnStateCallback cb) noexcept { m_partybtnstate_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_partybtnstate_cb && !m_partybtnstate_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<PartyBtnStateState>& State() const noexcept { return m_state; }
private:
 std::optional<PartyBtnStateState> m_state{};
 bool m_confirmed{};
 PartyBtnStateCallback m_partybtnstate_cb{};
};
}
