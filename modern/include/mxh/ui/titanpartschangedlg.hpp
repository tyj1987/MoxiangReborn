#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct TitanPartsChangeState { std::uint32_t titan_id{}; std::uint8_t part_kind{}; };
class cTitanPartsChangeDlg final : public cDialog {
public:
 using TitanPartsChangeCallback = std::function<bool(const TitanPartsChangeState&)>;
 bool Set(TitanPartsChangeState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetTitanPartsChangeCallback(TitanPartsChangeCallback cb) noexcept { m_titanpartschange_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_titanpartschange_cb && !m_titanpartschange_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<TitanPartsChangeState>& State() const noexcept { return m_state; }
private:
 std::optional<TitanPartsChangeState> m_state{};
 bool m_confirmed{};
 TitanPartsChangeCallback m_titanpartschange_cb{};
};
}
