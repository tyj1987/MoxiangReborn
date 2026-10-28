#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct CharMakeSubmitState { std::uint8_t race{}; std::uint8_t gender{}; std::uint8_t face_idx{}; std::uint8_t hair_idx{}; };
class cCharMakeDialog final : public cDialog {
public:
 using CharMakeSubmitCallback = std::function<bool(const CharMakeSubmitState&)>;
 bool Set(CharMakeSubmitState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetCharMakeSubmitCallback(CharMakeSubmitCallback cb) noexcept { m_charmakesubmit_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_charmakesubmit_cb && !m_charmakesubmit_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<CharMakeSubmitState>& State() const noexcept { return m_state; }
private:
 std::optional<CharMakeSubmitState> m_state{};
 bool m_confirmed{};
 CharMakeSubmitCallback m_charmakesubmit_cb{};
};
}
