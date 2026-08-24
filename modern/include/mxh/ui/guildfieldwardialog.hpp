#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct GuildFieldWarRequestState { std::uint32_t attacker_guild{}; std::uint32_t defender_guild{}; };
class cGuildFieldWarDialog final : public cDialog {
public:
 using GuildFieldWarRequestCallback = std::function<bool(const GuildFieldWarRequestState&)>;
 bool Set(GuildFieldWarRequestState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetGuildFieldWarRequestCallback(GuildFieldWarRequestCallback cb) noexcept { m_guildfieldwarrequest_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_guildfieldwarrequest_cb && !m_guildfieldwarrequest_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<GuildFieldWarRequestState>& State() const noexcept { return m_state; }
private:
 std::optional<GuildFieldWarRequestState> m_state{};
 bool m_confirmed{};
 GuildFieldWarRequestCallback m_guildfieldwarrequest_cb{};
};
}
