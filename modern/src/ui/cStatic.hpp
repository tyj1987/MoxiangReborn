// cStatic.hpp — modern port of 墨香 cStatic (label).
//
// 1:1 port of legacy `cStatic` from
//   `墨香【源码】\[Client]MH\interface\cStatic.h`.
//
// A cStatic is a non-interactive text label. It owns:
//   - a text buffer (UTF-8)
//   - foreground + shadow colors
//   - a font-index hint (legacy: tied to the engine's font table;
//     modern port: stored as opaque std::uint16_t for future render hook)
//   - text-position offset + alignment (TXT_LEFT/CENTER/RIGHT)
//   - a multi-line flag (used by the legacy's cMultiLineText)
//
// Render is a no-op (the actual draw goes through the 6.4+ cImage seam).

#pragma once

#include "cWindow.hpp"

#include <cstdint>
#include <string>

namespace mxh::ui {

class cStatic : public cWindow {
public:
    cStatic();
    ~cStatic() override;

    void SetStaticText(std::string text);
    void SetStaticValue(std::int32_t v);   // legacy itoa wrapper
    const std::string& GetStaticText() const noexcept { return m_text; }
    std::int32_t GetStaticValue() const noexcept;

    void SetFontIdx(std::uint16_t idx) noexcept { m_fontIdx = idx; }
    std::uint16_t GetFontIdx() const noexcept    { return m_fontIdx; }

    void SetMultiLine(bool v) noexcept           { m_multiLine = v; }
    bool IsMultiLine() const noexcept            { return m_multiLine; }

    void SetTextXY(std::int32_t x, std::int32_t y) noexcept {
        m_textX = x; m_textY = y;
    }
    void SetFGColor(std::uint32_t color) noexcept    { m_fgColor = color; }
    std::uint32_t GetFGColor() const noexcept        { return m_fgColor; }

    void SetShadow(bool v) noexcept                  { m_shadow = v; }
    bool HasShadow() const noexcept                  { return m_shadow; }
    void SetShadowTextXY(std::int32_t x, std::int32_t y) noexcept {
        m_shadowX = x; m_shadowY = y;
    }
    void SetShadowColor(std::uint32_t c) noexcept    { m_shadowColor = c; }

    enum class Align { Left = 0, Center = 1, Right = 2 };
    void SetAlign(Align a) noexcept                  { m_align = a; }
    Align GetAlign() const noexcept                  { return m_align; }

    // Render placeholder; the real draw goes through the 6.4+ adapter.
    void Render() override {}

private:
    std::string    m_text;
    std::uint16_t  m_fontIdx     = 0;
    bool           m_multiLine   = false;
    std::int32_t   m_textX       = 0;
    std::int32_t   m_textY       = 0;
    std::uint32_t  m_fgColor     = 0xFF000000;
    bool           m_shadow      = false;
    std::int32_t   m_shadowX     = 0;
    std::int32_t   m_shadowY     = 0;
    std::uint32_t  m_shadowColor = 0xFF000000;
    Align          m_align       = Align::Left;
};

} // namespace mxh::ui
