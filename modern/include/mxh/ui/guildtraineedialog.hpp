#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct GuildTraineeState { std::uint32_t trainee_id{}; std::uint8_t training_type{}; };
class cGuildTraineeDialog final : public cDialog {
public:
 using GuildTraineeCallback = std::function<bool(const GuildTraineeState&)>;
 bool Set(GuildTraineeState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetGuildTraineeCallback(GuildTraineeCallback cb) noexcept { m_guildtrainee_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_guildtrainee_cb && !m_guildtrainee_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<GuildTraineeState>& State() const noexcept { return m_state; }
private:
 std::optional<GuildTraineeState> m_state{};
 bool m_confirmed{};
 GuildTraineeCallback m_guildtrainee_cb{};
};
}
