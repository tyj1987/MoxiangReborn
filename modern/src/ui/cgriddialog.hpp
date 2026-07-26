#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct GridClickState { std::uint16_t row_count{}; std::uint16_t col_count{}; };
class cGridDialog final : public cDialog {
public:
 using GridClickCallback = std::function<bool(const GridClickState&)>;
 bool Set(GridClickState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetGridClickCallback(GridClickCallback cb) noexcept { m_gridclick_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_gridclick_cb && !m_gridclick_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<GridClickState>& State() const noexcept { return m_state; }
private:
 std::optional<GridClickState> m_state{};
 bool m_confirmed{};
 GridClickCallback m_gridclick_cb{};
};
}
