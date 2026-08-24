#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct TitanMixState { std::uint32_t recipe_id{}; std::uint32_t titan_id{}; };
class cTitanMixDlg final : public cDialog {
public:
 using TitanMixCallback = std::function<bool(const TitanMixState&)>;
 bool Set(TitanMixState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetTitanMixCallback(TitanMixCallback cb) noexcept { m_titanmix_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_titanmix_cb && !m_titanmix_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<TitanMixState>& State() const noexcept { return m_state; }
private:
 std::optional<TitanMixState> m_state{};
 bool m_confirmed{};
 TitanMixCallback m_titanmix_cb{};
};
}
