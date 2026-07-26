#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct GuildPlusTimeState { std::uint32_t guild_id{}; std::uint32_t time_seconds{}; };
class cGuildPlusTimeDialog final : public cDialog {
public:
 using GuildPlusTimeCallback = std::function<bool(const GuildPlusTimeState&)>;
 bool Set(GuildPlusTimeState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetGuildPlusTimeCallback(GuildPlusTimeCallback cb) noexcept { m_guildplustime_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_guildplustime_cb && !m_guildplustime_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<GuildPlusTimeState>& State() const noexcept { return m_state; }
private:
 std::optional<GuildPlusTimeState> m_state{};
 bool m_confirmed{};
 GuildPlusTimeCallback m_guildplustime_cb{};
};
}
