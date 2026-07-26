#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct UniqueItemMixState { std::uint32_t recipe_id{}; std::uint16_t item_count{}; };
class cUniqueItemMixDlg final : public cDialog {
public:
 using UniqueItemMixCallback = std::function<bool(const UniqueItemMixState&)>;
 bool Set(UniqueItemMixState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetUniqueItemMixCallback(UniqueItemMixCallback cb) noexcept { m_uniqueitemmix_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_uniqueitemmix_cb && !m_uniqueitemmix_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<UniqueItemMixState>& State() const noexcept { return m_state; }
private:
 std::optional<UniqueItemMixState> m_state{};
 bool m_confirmed{};
 UniqueItemMixCallback m_uniqueitemmix_cb{};
};
}
