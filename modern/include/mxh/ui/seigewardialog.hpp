#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct SeigeWarMatchState { std::uint8_t attack_team_id{}; std::uint8_t defend_team_id{}; std::uint32_t castle_id{}; };
class cSeigeWarDialog final : public cDialog {
public:
 using SeigeWarMatchCallback = std::function<bool(const SeigeWarMatchState&)>;
 bool Set(SeigeWarMatchState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetSeigeWarMatchCallback(SeigeWarMatchCallback cb) noexcept { m_seigewarmatch_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_seigewarmatch_cb && !m_seigewarmatch_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<SeigeWarMatchState>& State() const noexcept { return m_state; }
private:
 std::optional<SeigeWarMatchState> m_state{};
 bool m_confirmed{};
 SeigeWarMatchCallback m_seigewarmatch_cb{};
};
}
