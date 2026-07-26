#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct QuestTotalState { std::uint16_t active_count{}; std::uint16_t completed_count{}; };
class cQuestTotalDialog final : public cDialog {
public:
 using QuestTotalCallback = std::function<bool(const QuestTotalState&)>;
 bool Set(QuestTotalState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetQuestTotalCallback(QuestTotalCallback cb) noexcept { m_questtotal_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_questtotal_cb && !m_questtotal_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<QuestTotalState>& State() const noexcept { return m_state; }
private:
 std::optional<QuestTotalState> m_state{};
 bool m_confirmed{};
 QuestTotalCallback m_questtotal_cb{};
};
}
