// cPushupButton.hpp — modern port of 墨香 cPushupButton (toggle button).
//
// 1:1 port of legacy `cPushupButton` from
//   `墨香【源码】\[Client]MH\interface\cPushupButton.h`.
//
// A cPushupButton is a cButton that stays in the "pressed" state after
// the user clicks it; a second click releases it. This is the classic
// tab/sticky-button pattern used in dialogs that have multiple "pages"
// (e.g. Guild dialog's Member / Skill / Info tabs). The legacy engine
// also has a `m_fPassive` flag that disables user-toggle and lets code
// alone flip the state — used for the "currently visible" indicator
// when another tab's button is pressed.

#pragma once

#include "cButton.hpp"

namespace mxh::ui {

class cPushupButton : public cButton {
public:
    cPushupButton();
    ~cPushupButton() override;

    std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                              std::uint32_t mouseFlags) override;

    void  SetPush(bool v) noexcept;
    void  SetPushEx(bool v) noexcept;     // bypass the cButton::Init callback
    void  SetPassive(bool v) noexcept     { m_passive = v; }
    bool  IsPushed() const noexcept       { return m_pushed; }
    bool  IsPassive() const noexcept      { return m_passive; }

private:
    bool m_pushed  = false;
    bool m_passive = false;
};

} // namespace mxh::ui
