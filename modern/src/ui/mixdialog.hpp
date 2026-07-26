#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct MixRecipeState { std::uint32_t recipe_id{}; std::uint32_t result_item{}; std::uint32_t gold_required{}; };
class cMixDialog final : public cDialog {
public:
 using MixRecipeCallback = std::function<bool(const MixRecipeState&)>;
 bool Set(MixRecipeState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetMixRecipeCallback(MixRecipeCallback cb) noexcept { m_mixrecipe_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_mixrecipe_cb && !m_mixrecipe_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<MixRecipeState>& State() const noexcept { return m_state; }
private:
 std::optional<MixRecipeState> m_state{};
 bool m_confirmed{};
 MixRecipeCallback m_mixrecipe_cb{};
};
}
