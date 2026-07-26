#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct GuildMunhaUpdateState { std::uint32_t deposit_amount{}; std::uint32_t withdraw_amount{}; };
class cGuildMunhaDialog final : public cDialog {
public:
 using GuildMunhaUpdateCallback = std::function<bool(const GuildMunhaUpdateState&)>;
 bool Set(GuildMunhaUpdateState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetGuildMunhaUpdateCallback(GuildMunhaUpdateCallback cb) noexcept { m_guildmunhaupdate_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_guildmunhaupdate_cb && !m_guildmunhaupdate_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<GuildMunhaUpdateState>& State() const noexcept { return m_state; }
private:
 std::optional<GuildMunhaUpdateState> m_state{};
 bool m_confirmed{};
 GuildMunhaUpdateCallback m_guildmunhaupdate_cb{};
};
}
