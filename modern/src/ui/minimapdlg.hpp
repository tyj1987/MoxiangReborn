#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct MiniMapTickState { std::uint32_t map_id{}; std::uint8_t zoom_level{}; };
class cMiniMapDlg final : public cDialog {
public:
 using MiniMapTickCallback = std::function<bool(const MiniMapTickState&)>;
 bool Set(MiniMapTickState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetMiniMapTickCallback(MiniMapTickCallback cb) noexcept { m_minimaptick_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_minimaptick_cb && !m_minimaptick_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<MiniMapTickState>& State() const noexcept { return m_state; }
private:
 std::optional<MiniMapTickState> m_state{};
 bool m_confirmed{};
 MiniMapTickCallback m_minimaptick_cb{};
};
}
