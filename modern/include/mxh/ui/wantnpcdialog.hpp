#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct WantNpcPostState { std::uint32_t npc_id{}; std::uint32_t bounty{}; };
class cWantNpcDialog final : public cDialog {
public:
 using WantNpcPostCallback = std::function<bool(const WantNpcPostState&)>;
 bool Set(WantNpcPostState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetWantNpcPostCallback(WantNpcPostCallback cb) noexcept { m_wantnpcpost_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_wantnpcpost_cb && !m_wantnpcpost_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<WantNpcPostState>& State() const noexcept { return m_state; }
private:
 std::optional<WantNpcPostState> m_state{};
 bool m_confirmed{};
 WantNpcPostCallback m_wantnpcpost_cb{};
};
}
