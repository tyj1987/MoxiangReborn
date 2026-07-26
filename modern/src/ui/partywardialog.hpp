#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct PartyWarRequestState { std::uint32_t challenger_team{}; std::uint32_t target_team{}; };
class cPartyWarDialog final : public cDialog {
public:
 using PartyWarRequestCallback = std::function<bool(const PartyWarRequestState&)>;
 bool Set(PartyWarRequestState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetPartyWarRequestCallback(PartyWarRequestCallback cb) noexcept { m_partywarrequest_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_partywarrequest_cb && !m_partywarrequest_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<PartyWarRequestState>& State() const noexcept { return m_state; }
private:
 std::optional<PartyWarRequestState> m_state{};
 bool m_confirmed{};
 PartyWarRequestCallback m_partywarrequest_cb{};
};
}
