// cMultiLineText.hpp — modern port of 墨香 cMultiLineText (linked-list
// of text lines, used by NPC dialogs / system messages / quest dialogs).
//
// 1:1 port of legacy `cMultiLineText` from
//   `墨香【源码】\[Client]MH\interface\cMultiLineText.h`.
//
// The legacy uses cMultiLineText for any widget that displays multiple
// lines of text where each line can have its own color (NPC dialogs,
// quest descriptions, system messages, item tooltips). Internally it
// is a singly-linked list of LINE_NODE records (text buffer, length,
// color, next pointer). The modern port uses std::list to keep the
// semantics identical without the manual next-pointer dance, but
// exposes the same API (AddLine, SetText, SetFontIdx, SetXY, etc.).
//
// Render is a no-op (the actual draw is the render-layer's job — the
// 6.4+ cImage seam is the right integration point).

#pragma once

#include <cstdint>
#include <list>
#include <string>

namespace mxh::ui {

class cImage;

class cMultiLineText {
public:
    struct Line {
        std::string  text;
        std::uint32_t len    = 0;
        std::uint32_t color  = 0xFFFFFFFFu;
    };

    cMultiLineText();
    ~cMultiLineText();

    void Init(std::uint16_t fontIdx, std::uint32_t fgColor,
              cImage* bgImage = nullptr, std::uint32_t imgColor = 0xFFFFFFFFu);
    void Release() noexcept;

    // Set the entire text. The legacy splits on '\n' into multiple
    // LINE_NODEs; the modern port mirrors that.
    void SetText(const char* text);
    void SetText(const std::string& text) { SetText(text.c_str()); }

    // Add a single line at the tail with optional color.
    void AddLine(const char* text, std::uint32_t color = 0xFFFFFFFFu);
    void AddLine(const std::string& text, std::uint32_t color = 0xFFFFFFFFu);

    // Add a "name pannel" placeholder: the legacy's AddNamePannel
    // creates a colored bar at the top used for player name labels in
    // chat. We represent it as a single line of spaces with the
    // given length, which the render layer can interpret as a bar.
    void AddNamePannel(std::uint32_t dwLength) noexcept;

    bool IsValid() const noexcept { return m_valid; }

    void SetFontIdx(std::uint16_t idx) noexcept { m_fontIdx = idx; }
    void SetFGColor(std::uint32_t c) noexcept    { m_fgColor = c; }
    void SetXY(std::int32_t x, std::int32_t y) noexcept {
        m_x = x; m_y = y;
    }
    void SetImageRGB(std::uint32_t c) noexcept       { m_imgColor = c; }
    void SetImageAlpha(std::uint32_t a) noexcept     { m_alpha = a; }
    void SetOptionAlpha(std::uint32_t a) noexcept    { m_optionAlpha = a; }

    // Accessors.
    std::uint16_t GetFontIdx() const noexcept        { return m_fontIdx; }
    std::uint32_t GetFGColor() const noexcept        { return m_fgColor; }
    std::int32_t  GetX() const noexcept              { return m_x; }
    std::int32_t  GetY() const noexcept              { return m_y; }
    std::size_t   LineCount() const noexcept         { return m_lines.size(); }
    bool          Empty() const noexcept             { return m_lines.empty(); }

    const Line& GetLine(std::size_t i) const;
    const std::list<Line>& Lines() const noexcept     { return m_lines; }

    void operator=(const char* text) { SetText(text); }

    // Render placeholder; the real draw goes through the 6.4+ adapter.
    void Render() {}

private:
    std::list<Line>    m_lines;
    bool               m_valid         = false;
    std::uint16_t      m_fontIdx       = 0;
    cImage*            m_bgImage       = nullptr;
    std::uint32_t      m_fgColor       = 0xFF000000;
    std::uint32_t      m_imgColor      = 0xFFFFFFFFu;
    std::int32_t       m_x             = 0;
    std::int32_t       m_y             = 0;
    std::uint32_t      m_alpha         = 0xFF000000;
    std::uint32_t      m_optionAlpha   = 0xFF000000;
    bool               m_hasNamePannel = false;
};

} // namespace mxh::ui
