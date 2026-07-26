#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct SkillOptionChangeState { std::uint32_t mugong_idx{}; std::uint8_t option_kind{}; };
class cSkillOptionChangeDlg final : public cDialog {
public:
 using SkillOptionChangeCallback = std::function<bool(const SkillOptionChangeState&)>;
 bool Set(SkillOptionChangeState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetSkillOptionChangeCallback(SkillOptionChangeCallback cb) noexcept { m_skilloptionchange_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_skilloptionchange_cb && !m_skilloptionchange_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<SkillOptionChangeState>& State() const noexcept { return m_state; }
private:
 std::optional<SkillOptionChangeState> m_state{};
 bool m_confirmed{};
 SkillOptionChangeCallback m_skilloptionchange_cb{};
};
}
