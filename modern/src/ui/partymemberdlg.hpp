#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct PartyMemberRefreshState { std::uint8_t member_count{}; };
class cPartyMemberDlg final : public cDialog {
public:
 using PartyMemberRefreshCallback = std::function<bool(const PartyMemberRefreshState&)>;
 bool Set(PartyMemberRefreshState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetPartyMemberRefreshCallback(PartyMemberRefreshCallback cb) noexcept { m_partymemberrefresh_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_partymemberrefresh_cb && !m_partymemberrefresh_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<PartyMemberRefreshState>& State() const noexcept { return m_state; }
private:
 std::optional<PartyMemberRefreshState> m_state{};
 bool m_confirmed{};
 PartyMemberRefreshCallback m_partymemberrefresh_cb{};
};
}
