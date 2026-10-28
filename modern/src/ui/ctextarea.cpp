// ctextarea.cpp — 1:1 port of 墨香 cTextArea (multi-line
// text area). See ctextarea.hpp for the data-model
// rationale + 1:1 quirks.

#include "ctextarea.hpp"
#include "cImage.hpp"
#include "TextRender.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

namespace {
std::size_t prev_utf8(const std::string& s, std::size_t pos) {
    if (pos == 0) return 0;
    --pos;
    while (pos > 0 && (static_cast<unsigned char>(s[pos]) & 0xC0u) == 0x80u) --pos;
    return pos;
}

std::size_t next_utf8(const std::string& s, std::size_t pos) {
    if (pos >= s.size()) return s.size();
    ++pos;
    while (pos < s.size() && (static_cast<unsigned char>(s[pos]) & 0xC0u) == 0x80u) ++pos;
    return pos;
}

std::string encode_utf8(std::int32_t cp) {
    if (cp < 0 || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return {};
    std::string out;
    if (cp <= 0x7F) out.push_back(static_cast<char>(cp));
    else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    return out;
}
}

namespace mxh::ui {

cTextArea::cTextArea() = default;

cTextArea::~cTextArea() = default;

void cTextArea::InitTextArea(const TextRect& textRelRect, int bufSize,
                             void* topImage, std::uint16_t topHeight,
                             void* middleImage, std::uint16_t middleHeight,
                             void* downImage, std::uint16_t downHeight) {
    // 1:1 with legacy cTextArea::InitTextArea (full
    // overload). The legacy stores the 3 chrome images
    // + heights + text rect + buffer size. Modern
    // port stores the same fields.
    m_rcTextRelRect = textRelRect;
    m_TopImage     = topImage;
    m_topHeight    = topHeight;
    m_MiddleImage  = middleImage;
    m_middleHeight = middleHeight;
    m_DownImage    = downImage;
    m_downHeight   = downHeight;
    // 1:1 quirk: legacy stores m_nBufSize (the max
    // text buffer size). Modern port derives the
    // max line count from it (m_nMaxLine = bufSize
    // as a stand-in for the line cap).
    m_nMaxLine     = bufSize;
}

void cTextArea::InitTextArea(const TextRect& textRelRect, int bufSize) {
    // 1:1 with legacy cTextArea::InitTextArea (simple
    // overload). The simple overload sets the text
    // rect + buffer size; the 3 chrome images stay
    // null (the host can set them later via
    // SetMiddleScale or via cDialog::Add).
    m_rcTextRelRect = textRelRect;
    m_nMaxLine      = bufSize;
}

void cTextArea::SetActive(bool val) noexcept {
    // 1:1 with legacy cTextArea::SetActive. The legacy
    // toggles the caret visibility based on the new
    // active state. Modern port: calls base SetActive
    // + stores the caret intent (m_bCaret = val).
    // Actual caret blink / render is Phase 6.13+
    // deferred.
    cDialog::SetActive(val);
    m_bCaret = val;
}

void cTextArea::SetFocusEdit(bool val) noexcept {
    // 1:1 with legacy cTextArea::SetFocusEdit. Stores
    // the focus state — actual caret positioning +
    // render is Phase 12.x deferred.
    m_bCaret = val;
}

std::uint32_t cTextArea::ActionEvent(std::int32_t mouseX,
                                     std::int32_t mouseY,
                                     std::uint32_t mouseFlags) {
    if (!isEnabled() || !isVisible() || !isActive()) {
        return static_cast<std::uint32_t>(WindowEvent::Null);
    }
    const auto childEvent = cDialog::ActionEvent(mouseX, mouseY, mouseFlags);
    const auto& r = m_rcTextRelRect;
    const bool inside = mouseX >= absX() + r.left &&
                        mouseX <= absX() + r.right &&
                        mouseY >= absY() + r.top &&
                        mouseY <= absY() + r.bottom;
    if (!inside) return childEvent;
    if (mouseFlags & MouseFlagLButton) {
        SetFocusEdit(true);
        return static_cast<std::uint32_t>(WindowEvent::LButtonClick);
    }
    return childEvent;
}

std::uint32_t cTextArea::ActionKeyboardEvent(std::int32_t key,
                                             std::int32_t ch) {
    if (!isEnabled() || !isActive() || !m_bCaret) {
        return static_cast<std::uint32_t>(WindowEvent::Null);
    }
    constexpr std::int32_t kBackspace = 8;
    constexpr std::int32_t kEnter = 13;
    if (key == kBackspace) {
        if (!m_bReadOnly && m_caretPos > 0) {
            const auto begin = prev_utf8(m_scriptText, m_caretPos);
            m_scriptText.erase(begin, m_caretPos - begin);
            m_caretPos = begin;
        }
        return static_cast<std::uint32_t>(WindowEvent::KeyDown);
    }
    if (key == 37) { m_caretPos = prev_utf8(m_scriptText, m_caretPos); return static_cast<std::uint32_t>(WindowEvent::KeyDown); }
    if (key == 39) { m_caretPos = next_utf8(m_scriptText, m_caretPos); return static_cast<std::uint32_t>(WindowEvent::KeyDown); }
    if (key == 36) { m_caretPos = 0; return static_cast<std::uint32_t>(WindowEvent::KeyDown); }
    if (key == 35) { m_caretPos = m_scriptText.size(); return static_cast<std::uint32_t>(WindowEvent::KeyDown); }
    if (key == 46) {
        if (!m_bReadOnly && m_caretPos < m_scriptText.size()) {
            m_scriptText.erase(m_caretPos, next_utf8(m_scriptText, m_caretPos) - m_caretPos);
        }
        return static_cast<std::uint32_t>(WindowEvent::KeyDown);
    }
    if (key == kEnter) {
        if (!m_bReadOnly && m_bEnterAllow) {
            if (m_nMaxLine <= 0 ||
                static_cast<int>(m_scriptText.size()) < m_nMaxLine) {
                m_scriptText.insert(m_caretPos, 1, '\n');
                ++m_caretPos;
            }
        }
        return static_cast<std::uint32_t>(WindowEvent::KeyDown);
    }
    if (ch > 0) {
        const auto encoded = encode_utf8(ch);
        if (!encoded.empty() && !m_bReadOnly && (m_nMaxLine <= 0 ||
                             static_cast<int>(m_scriptText.size() + encoded.size()) < m_nMaxLine)) {
            m_scriptText.insert(m_caretPos, encoded);
            m_caretPos += encoded.size();
        }
        return static_cast<std::uint32_t>(WindowEvent::Char_);
    }
    return static_cast<std::uint32_t>(WindowEvent::Null);
}

void cTextArea::SetScriptText(const char* inText) {
    // 1:1 with legacy cTextArea::SetScriptText. The
    // legacy stores the text in an internal buffer
    // (caller is responsible for the lifetime). Modern
    // port uses std::string for safe storage.
    if (inText) m_scriptText = inText;
    else        m_scriptText.clear();
    m_nTopLineIdx = 0;
    m_caretPos = m_bCaretMoveFirst ? 0 : m_scriptText.size();
}

void cTextArea::OnUpwardItem() noexcept {
    if (m_nTopLineIdx > 0) --m_nTopLineIdx;
}

void cTextArea::OnDownwardItem() noexcept {
    const int visible = (m_nLineHeight > 0 && m_rcTextRelRect.bottom > m_rcTextRelRect.top)
        ? (m_rcTextRelRect.bottom - m_rcTextRelRect.top) / m_nLineHeight : 1;
    int lines = 1;
    for (const char ch : m_scriptText) if (ch == '\n') ++lines;
    const int maxTop = std::max(0, lines - std::max(1, visible));
    if (m_nTopLineIdx < maxTop) ++m_nTopLineIdx;
}

void cTextArea::GetScriptTextCString(char* outText, int bufSize) const {
    // 1:1 quirk: legacy GetScriptText copies the text
    // into a caller-provided buffer (c-style). Modern
    // port exposes this for legacy callers that need
    // c-string compatibility.
    if (!outText || bufSize <= 0) return;
    std::strncpy(outText, m_scriptText.c_str(),
                 static_cast<std::size_t>(bufSize - 1));
    outText[bufSize - 1] = '\0';
}

bool cTextArea::SetLimitLine(int nMaxLine) noexcept {
    // 1:1 with legacy cTextArea::SetLimitLine. Returns
    // true on success. Reject negative line counts.
    if (nMaxLine < 0) return false;
    m_nMaxLine = nMaxLine;
    return true;
}

void cTextArea::Add(cWindow* window) {
    // 1:1 with legacy cTextArea::Add. The legacy
    // override just calls cDialog::Add. Modern port
    // delegates via unique_ptr (cDialog::Add takes
    // ownership). We assume the caller transfers
    // ownership (1:1 with the legacy raw-pointer
    // ownership convention).
    if (!window) return;
    cDialog::Add(std::unique_ptr<cWindow>(window));
}

void cTextArea::Render() {
    if (!isVisible()) return;
    cDialog::Render();

    const auto drawImage = [](void* handle, std::int32_t x, std::int32_t y,
                              std::int32_t width, std::int32_t height) {
        if (!handle || width <= 0 || height <= 0) return;
        static_cast<cImage*>(handle)->render(x, y, width, height,
                                             0xFFFFFFFFu, 1);
    };
    const auto originX = absX();
    const auto originY = absY();
    const auto areaWidth = static_cast<std::int32_t>(width());
    if (m_topHeight) drawImage(m_TopImage, originX, originY, areaWidth, m_topHeight);

    const auto bodyY = originY + static_cast<std::int32_t>(m_topHeight);
    const auto bodyHeight = std::max<std::int32_t>(
        0, static_cast<std::int32_t>(height()) - m_topHeight - m_downHeight);
    if (bodyHeight > 0 && m_middleHeight > 0) {
        for (std::int32_t y = 0; y < bodyHeight; y += m_middleHeight) {
            drawImage(m_MiddleImage, originX, bodyY + y, areaWidth,
                      std::min<std::int32_t>(m_middleHeight, bodyHeight - y));
        }
    }
    if (m_downHeight)
        drawImage(m_DownImage, originX,
                  originY + static_cast<std::int32_t>(height()) - m_downHeight,
                  areaWidth, m_downHeight);

    const auto left = originX + m_rcTextRelRect.left;
    const auto top = originY + m_rcTextRelRect.top;
    const auto textWidth = std::max<std::int32_t>(0,
        m_rcTextRelRect.right - m_rcTextRelRect.left);
    const auto textHeight = std::max<std::int32_t>(0,
        m_rcTextRelRect.bottom - m_rcTextRelRect.top);
    if (textWidth == 0 || textHeight == 0) return;

    std::vector<std::string> lines;
    std::size_t begin = 0;
    while (begin <= m_scriptText.size()) {
        const auto end = m_scriptText.find('\n', begin);
        lines.push_back(m_scriptText.substr(begin,
            end == std::string::npos ? std::string::npos : end - begin));
        if (end == std::string::npos) break;
        begin = end + 1;
    }
    const auto first = std::clamp(m_nTopLineIdx, 0,
                                  static_cast<int>(lines.size()));
    const auto visible = std::max(0, textHeight / std::max(1, m_nLineHeight));
    for (int i = 0; i < visible && first + i < static_cast<int>(lines.size()); ++i) {
        std::string line = lines[static_cast<std::size_t>(first + i)];
        if (m_bCaret && first + i == static_cast<int>(lines.size()) - 1)
            line += "|";
        TextRenderRequest request;
        request.text = line;
        request.x = left;
        request.y = top + i * m_nLineHeight;
        request.width = textWidth;
        request.height = m_nLineHeight;
        request.color = m_dwTextColor;
        request.font_index = 0;
        request.align = TextRenderAlign::Left;
        renderText(request);
    }
}

}  // namespace mxh::ui
