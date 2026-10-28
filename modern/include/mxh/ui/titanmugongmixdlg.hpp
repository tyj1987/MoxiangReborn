#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct TitanMugongMixState { std::uint32_t titan_id{}; std::uint32_t mugong_idx{}; };
class cTitanMugongMixDlg final : public cDialog {
public:
 using TitanMugongMixCallback = std::function<bool(const TitanMugongMixState&)>;
 bool Set(TitanMugongMixState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetTitanMugongMixCallback(TitanMugongMixCallback cb) noexcept { m_titanmugongmix_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_titanmugongmix_cb && !m_titanmugongmix_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<TitanMugongMixState>& State() const noexcept { return m_state; }
private:
 std::optional<TitanMugongMixState> m_state{};
 bool m_confirmed{};
 TitanMugongMixCallback m_titanmugongmix_cb{};
};
}
