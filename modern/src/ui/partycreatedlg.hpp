#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct PartyCreateRequestState { std::uint8_t party_name_len{}; std::uint8_t is_public{}; };
class cPartyCreateDlg final : public cDialog {
public:
 using PartyCreateRequestCallback = std::function<bool(const PartyCreateRequestState&)>;
 bool Set(PartyCreateRequestState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetPartyCreateRequestCallback(PartyCreateRequestCallback cb) noexcept { m_partycreaterequest_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_partycreaterequest_cb && !m_partycreaterequest_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<PartyCreateRequestState>& State() const noexcept { return m_state; }
private:
 std::optional<PartyCreateRequestState> m_state{};
 bool m_confirmed{};
 PartyCreateRequestCallback m_partycreaterequest_cb{};
};
}
