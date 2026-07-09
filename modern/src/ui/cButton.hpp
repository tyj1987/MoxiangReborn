// mxh/ui/cButton.hpp
// Phase 6.1 — modern C++ cButton widget. Builds on cWindow (Phase 6.0).
//
// Surface intent: keep the legacy 1:1 cButton API close (3-state images,
// text label with 3 colors, click detection) but expressed in modern C++
// with std::function callbacks and CPU-side testable state. The real GPU
// draw path (cImage -> mxh_render SpriteObject) lands in 6.3; this
// skeleton exercises the state machine + click dispatch only.
//
// State machine:
//
//                          MouseOver (cursor enters box)
//   Normal  ─────────────────────────────────────────►  Hover
//      ▲                                                    │
//      │                       LButtonUp outside            │ LButtonDown
//      │  LButtonUp inside                                   ▼
//   Released◄──────────────── Pressed ◄──────────────────────┘
//                              │
//                              │ LButtonUp outside
//                              ▼
//                          Hover (no click fires)
//
// The `Clicked` window event fires only when the cursor was pressed AND
// released inside the button bounding box (the legacy engine's "real click"
// contract — accidentally dragging out cancels the click).
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <utility>

#include "cWindow.hpp"

namespace mxh::ui {

class cButton : public cWindow {
public:
    // Visual / state color for the text label. Stored as 0xAARRGGBB.
    using ColorRGBA = std::uint32_t;
    static constexpr ColorRGBA ColorDefault = 0xFF000000u;

    // 0 = left, 1 = center, 2 = right (mirrors legacy m_nAlign).
    enum class TextAlign : std::int32_t { Left = 0, Center = 1, Right = 2 };

    // Click callback (legacy: cbFUNC Func). The callback receives the
    // button's id and a void* userdata. Pure C++ form via std::function.
    using ClickCallback = std::function<void(std::int32_t buttonId, void* userdata)>;

    cButton() = default;
    ~cButton() override = default;

    cButton(const cButton&) = delete;
    cButton& operator=(const cButton&) = delete;

    // -------------------------------------------------------------------------
    // Init: position, size, three state images (opaque void* — cImage comes in
    // 6.3), the click callback, and the widget id. Mirrors the legacy
    // cButton::Init signature with the addition of a `userdata` pointer that
    // gets passed back to the callback.
    // -------------------------------------------------------------------------
    void Init(std::int32_t x, std::int32_t y, std::uint16_t wid, std::uint16_t hei,
              void* basicImage, void* overImage, void* pressImage,
              ClickCallback onClick = {}, void* userdata = nullptr,
              std::int32_t id = 0);

    // Render placeholder — real GPU draw lives in 6.3.
    void Render() override {}

    // ActionEvent intercepts the click state machine before delegating to
    // cWindow::ActionEvent for the default no-op path. The returned WE_*
    // code reflects the button's own state, not the generic window event.
    std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                              std::uint32_t mouseFlags) override;

    // -------------------------------------------------------------------------
    // Text label. `text` is stored as std::string; the legacy engine used a
    // fixed-size char[]. Real font rendering lands with cImage (6.3).
    // -------------------------------------------------------------------------
    void SetText(std::string text, ColorRGBA basic = ColorDefault,
                 ColorRGBA over = 0, ColorRGBA press = 0);
    const std::string& text() const noexcept { return m_text; }
    ColorRGBA textBasicColor() const noexcept { return m_fgBasicColor; }
    ColorRGBA textOverColor()  const noexcept { return m_fgOverColor; }
    ColorRGBA textPressColor() const noexcept { return m_fgPressColor; }
    ColorRGBA currentTextColor() const noexcept { return m_fgCurColor; }

    void SetTextXY(std::int32_t x, std::int32_t y) noexcept;
    void SetShadowTextXY(std::int32_t x, std::int32_t y) noexcept;
    void SetShadowColor(ColorRGBA c) noexcept   { m_shadowColor = c; }
    void SetShadow(bool v) noexcept              { m_bShadow    = v; }
    void SetTextAlign(TextAlign a) noexcept     { m_align      = a; }
    TextAlign textAlign() const noexcept         { return m_align; }

    // -------------------------------------------------------------------------
    // State introspection. The state machine is the source of truth for
    // "which image should I draw" — tests assert the state transitions.
    // -------------------------------------------------------------------------
    enum class State : std::int32_t { Normal = 0, Hover = 1, Pressed = 2, Disabled = 3 };
    State state() const noexcept { return m_state; }

    // Click was consumed since the last time the flag was checked.
    // Legacy: IsClickInside. Returns true once after a successful click,
    // then resets to false on read.
    bool consumeClickInside() noexcept;

    // Disable / enable the button. While disabled, all events are ignored.
    void SetEnabled(bool v) noexcept;

    // -------------------------------------------------------------------------
    // Test accessors (so unit tests can poke the state without going through
    // ActionEvent).
    // -------------------------------------------------------------------------
    void* basicImage()  const noexcept { return m_basicImage; }
    void* overImage()   const noexcept { return m_overImage; }
    void* pressImage()  const noexcept { return m_pressImage; }
    void* userdata()    const noexcept { return m_userdata; }
    bool  shadowEnabled() const noexcept { return m_bShadow; }
    ColorRGBA shadowColor() const noexcept { return m_shadowColor; }

private:
    // The three state images (opaque in skeleton; cImage* in 6.3).
    void* m_basicImage = nullptr;
    void* m_overImage  = nullptr;
    void* m_pressImage = nullptr;

    // Click callback.
    ClickCallback m_onClick;
    void*         m_userdata = nullptr;

    // Text + colors.
    std::string m_text;
    ColorRGBA   m_fgBasicColor = ColorDefault;
    ColorRGBA   m_fgOverColor  = 0;
    ColorRGBA   m_fgPressColor = 0;
    ColorRGBA   m_fgCurColor   = ColorDefault;
    std::int32_t m_textX = 0;
    std::int32_t m_textY = 0;

    // Shadow text.
    std::int32_t m_shadowTextX = 0;
    std::int32_t m_shadowTextY = 0;
    ColorRGBA    m_shadowColor = 0;
    bool         m_bShadow     = false;

    TextAlign    m_align = TextAlign::Center;

    // State machine.
    State m_state = State::Normal;
    bool  m_bClickInside = false;  // legacy: latched on successful click

    void transitionTo(State newState);
    void recomputeCurrentTextColor();
};

} // namespace mxh::ui
