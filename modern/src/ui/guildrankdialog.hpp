#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct GuildRankUpdateState { std::uint8_t rank_level{}; std::uint32_t member_count{}; };
class cGuildRankDialog final : public cDialog {
public:
 using GuildRankUpdateCallback = std::function<bool(const GuildRankUpdateState&)>;
 bool Set(GuildRankUpdateState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetGuildRankUpdateCallback(GuildRankUpdateCallback cb) noexcept { m_guildrankupdate_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_guildrankupdate_cb && !m_guildrankupdate_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<GuildRankUpdateState>& State() const noexcept { return m_state; }
private:
 std::optional<GuildRankUpdateState> m_state{};
 bool m_confirmed{};
 GuildRankUpdateCallback m_guildrankupdate_cb{};
};
}
