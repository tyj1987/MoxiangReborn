#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct BigMapClickState { std::uint32_t map_id{}; std::int32_t center_x{}; std::int32_t center_y{}; };
class cBigMapDlg final : public cDialog {
public:
 using BigMapClickCallback = std::function<bool(const BigMapClickState&)>;
 bool Set(BigMapClickState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetBigMapClickCallback(BigMapClickCallback cb) noexcept { m_bigmapclick_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_bigmapclick_cb && !m_bigmapclick_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<BigMapClickState>& State() const noexcept { return m_state; }
private:
 std::optional<BigMapClickState> m_state{};
 bool m_confirmed{};
 BigMapClickCallback m_bigmapclick_cb{};
};
}
