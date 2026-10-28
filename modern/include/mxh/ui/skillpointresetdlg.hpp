#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct SkillPointResetState { std::uint32_t character_id{}; std::uint8_t reset_kind{}; };
class cSkillPointResetDlg final : public cDialog {
public:
 using SkillPointResetCallback = std::function<bool(const SkillPointResetState&)>;
 bool Set(SkillPointResetState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetSkillPointResetCallback(SkillPointResetCallback cb) noexcept { m_skillpointreset_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_skillpointreset_cb && !m_skillpointreset_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<SkillPointResetState>& State() const noexcept { return m_state; }
private:
 std::optional<SkillPointResetState> m_state{};
 bool m_confirmed{};
 SkillPointResetCallback m_skillpointreset_cb{};
};
}
