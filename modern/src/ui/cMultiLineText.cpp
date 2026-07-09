// cMultiLineText.cpp — modern implementation of 墨香 cMultiLineText.

#include "cMultiLineText.hpp"

#include <cstring>

namespace mxh::ui {

cMultiLineText::cMultiLineText() = default;
cMultiLineText::~cMultiLineText() {
    Release();
}

void cMultiLineText::Init(std::uint16_t fontIdx, std::uint32_t fgColor,
                          cImage* bgImage, std::uint32_t imgColor) {
    Release();
    m_fontIdx  = fontIdx;
    m_fgColor  = fgColor;
    m_bgImage  = bgImage;
    m_imgColor = imgColor;
    m_valid    = true;
}

void cMultiLineText::Release() noexcept {
    m_lines.clear();
    m_valid = false;
}

void cMultiLineText::SetText(const char* text) {
    m_lines.clear();
    m_hasNamePannel = false;
    if (!text || *text == '\0') return;
    // Split on '\n' into multiple lines. The legacy treats the input
    // as a sequence of "lines" — the trailing newline that some text
    // editors leave behind should NOT produce a phantom empty line at
    // the end. We achieve that by not pushing the segment after the
    // final \n.
    const char* p = text;
    const char* lineStart = p;
    while (true) {
        if (*p == '\n') {
            Line ln;
            ln.text.assign(lineStart, p - lineStart);
            ln.len  = static_cast<std::uint32_t>(ln.text.size());
            ln.color = m_fgColor;
            m_lines.push_back(std::move(ln));
            ++p;
            lineStart = p;
            continue;
        }
        if (*p == '\0') {
            if (p != lineStart) {  // skip a trailing-empty segment
                Line ln;
                ln.text.assign(lineStart, p - lineStart);
                ln.len  = static_cast<std::uint32_t>(ln.text.size());
                ln.color = m_fgColor;
                m_lines.push_back(std::move(ln));
            }
            break;
        }
        ++p;
    }
}

void cMultiLineText::AddLine(const char* text, std::uint32_t color) {
    if (!text) return;
    Line ln;
    ln.text  = text;
    ln.len   = static_cast<std::uint32_t>(std::strlen(text));
    ln.color = color;
    m_lines.push_back(std::move(ln));
}

void cMultiLineText::AddLine(const std::string& text, std::uint32_t color) {
    Line ln;
    ln.text  = text;
    ln.len   = static_cast<std::uint32_t>(text.size());
    ln.color = color;
    m_lines.push_back(std::move(ln));
}

void cMultiLineText::AddNamePannel(std::uint32_t dwLength) noexcept {
    // The legacy creates a name panel as a colored bar of `dwLength`
    // characters (e.g. spaces with a background color). We represent
    // it as a single line of spaces with length=dwLength and a flag
    // the render layer can interpret.
    Line ln;
    ln.text.assign(dwLength, ' ');
    ln.len   = dwLength;
    ln.color = m_fgColor;
    m_lines.push_back(std::move(ln));
    m_hasNamePannel = true;
}

const cMultiLineText::Line& cMultiLineText::GetLine(std::size_t i) const {
    static const Line kEmpty{};
    if (i >= m_lines.size()) return kEmpty;
    auto it = m_lines.begin();
    for (std::size_t k = 0; k < i; ++k) ++it;
    return *it;
}

} // namespace mxh::ui
