// mxh/ui/cEditBox.cpp
// Phase 6.2 — implementation of the modern cEditBox widget.
#include "cEditBox.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>

#include "TextRender.hpp"

namespace mxh::ui {

namespace {
bool is_utf8_continuation(unsigned char byte) noexcept {
    return (byte & 0xC0u) == 0x80u;
}

std::size_t complete_utf8_prefix(const std::string& text,
                                 std::size_t limit) noexcept {
    const auto size = std::min(text.size(), limit);
    std::size_t pos = 0;
    while (pos < size) {
        const auto lead = static_cast<unsigned char>(text[pos]);
        std::size_t width = 1;
        if (lead >= 0xC2u && lead <= 0xDFu) width = 2;
        else if (lead >= 0xE0u && lead <= 0xEFu) width = 3;
        else if (lead >= 0xF0u && lead <= 0xF4u) width = 4;
        if (pos + width > size) break;
        bool valid = true;
        for (std::size_t i = 1; i < width; ++i) {
            if (!is_utf8_continuation(static_cast<unsigned char>(text[pos + i]))) {
                valid = false;
                break;
            }
        }
        if (!valid) break;
        pos += width;
    }
    return pos;
}

std::size_t utf8_codepoint_count(const std::string& text) noexcept {
    std::size_t count = 0;
    std::size_t pos = 0;
    while (pos < text.size()) {
        const auto lead = static_cast<unsigned char>(text[pos]);
        std::size_t width = 1;
        if (lead >= 0xC2u && lead <= 0xDFu) width = 2;
        else if (lead >= 0xE0u && lead <= 0xEFu) width = 3;
        else if (lead >= 0xF0u && lead <= 0xF4u) width = 4;
        if (pos + width > text.size()) width = 1;
        for (std::size_t i = 1; i < width; ++i) {
            if (!is_utf8_continuation(static_cast<unsigned char>(text[pos + i]))) {
                width = 1;
                break;
            }
        }
        pos += width;
        ++count;
    }
    return count;
}
}

void cEditBox::Init(std::int32_t x, std::int32_t y, std::uint16_t wid,
                    std::uint16_t hei, void* basicImage, void* focusImage,
                    std::int32_t id) {
    cWindow::Init(x, y, wid, hei, basicImage, id);
    m_basicImage = basicImage;
    m_focusImage = focusImage;
    SetFocus(false);
}

void cEditBox::InitEditbox(std::uint16_t /*pixelWidth*/, std::uint16_t bufBytes) {
    if (bufBytes < 2) bufBytes = 2;  // at least one char + NUL
    m_maxBytes  = bufBytes;
    m_text.clear();
    m_caret = 0;
    m_bTextChanged = 0;
}

void cEditBox::Render() {
    if (!isVisible()) return;

    SetBasicImage(hasFocus() && m_focusImage ? m_focusImage : m_basicImage);
    cWindow::Render();

    const auto text = displayText();
    const bool show_caret = hasFocus() && m_bCaret &&
        (!m_bReadOnly || m_bShowCaretInReadOnly);
    if (text.empty() && !show_caret) return;

    TextRenderRequest request;
    request.text = text;
    request.x = absX();
    request.y = absY() + m_textTopOffset;
    request.width = width();
    request.height = height();
    request.left_inset = m_textLeftOffset;
    request.right_inset = m_textRightOffset;
    request.color = hasFocus() ? m_activeTextColor : m_nonactiveTextColor;
    request.font_index = m_fontIdx;
    request.align = static_cast<TextRenderAlign>(m_align);
    if (show_caret) {
        request.caret_byte = m_caret;
    }
    renderText(request);
}

void cEditBox::SetEditText(std::string text) {
    if (m_maxBytes == 0) {
        // No buffer configured; legacy engine would treat this as a
        // configuration error. Be strict — refuse to set text before
        // InitEditbox is called.
        return;
    }
    // Truncate to fit (reserving 1 byte for NUL conceptually; we use
    // std::string and cap to m_maxBytes - 1).
    const std::size_t cap = m_maxBytes > 0 ? m_maxBytes - 1 : 0;
    if (text.size() > cap) text.resize(cap);
    text.resize(complete_utf8_prefix(text, text.size()));
    m_text  = std::move(text);
    m_caret = m_text.size();
    fireChange();
}

void cEditBox::ClearEditTextSecure() noexcept {
    volatile char* bytes = m_text.empty() ? nullptr : m_text.data();
    for (std::size_t i = 0; bytes && i < m_text.size(); ++i) {
        bytes[i] = '\0';
    }
    m_text.clear();
    m_caret = 0;
    fireChange();
}

std::string cEditBox::displayText() const {
    if (!m_bSecret) return m_text;
    return std::string(utf8_codepoint_count(m_text), '*');
}

void cEditBox::SetCaretPos(std::size_t pos) noexcept {
    if (pos > m_text.size()) pos = m_text.size();
    while (pos > 0 && pos < m_text.size() &&
           is_utf8_continuation(static_cast<unsigned char>(m_text[pos]))) {
        --pos;
    }
    m_caret = pos;
}

void cEditBox::SetTextOffset(std::int32_t left, std::int32_t right,
                             std::int32_t top) noexcept {
    m_textLeftOffset  = left;
    m_textRightOffset = right;
    m_textTopOffset   = top;
}

void cEditBox::insertCharAtCaret(char c) {
    if (m_maxBytes == 0) return;
    if (!charAllowed(c)) return;

    // Capacity: m_maxBytes is the total buffer; std::string size() is
    // the byte count without NUL. Reserve one byte for the implicit NUL.
    const std::size_t cap = m_maxBytes - 1;
    if (m_bInsert) {
        if (m_text.size() >= cap) return;
        m_text.insert(m_text.begin() + static_cast<std::ptrdiff_t>(m_caret), c);
        ++m_caret;
    } else {
        // Overwrite mode: replace the char at the caret, or append if
        // the caret is at the end.
        if (m_caret < m_text.size()) {
            m_text[m_caret] = c;
            ++m_caret;
        } else {
            if (m_text.size() >= cap) return;
            m_text.push_back(c);
            ++m_caret;
        }
    }
    fireChange();
}

void cEditBox::insertCodepointAtCaret(std::uint32_t codepoint) {
    if (m_maxBytes == 0 || codepoint == 0 || codepoint < 0x20u ||
        codepoint == 0x7Fu || codepoint > 0x10FFFFu ||
        (codepoint >= 0xD800u && codepoint <= 0xDFFFu)) return;
    if (m_validCheck != 0 && codepoint > 0x7Fu) return;

    char encoded[4]{};
    std::size_t count = 0;
    if (codepoint <= 0x7Fu) {
        encoded[count++] = static_cast<char>(codepoint);
    } else if (codepoint <= 0x7FFu) {
        encoded[count++] = static_cast<char>(0xC0u | (codepoint >> 6));
        encoded[count++] = static_cast<char>(0x80u | (codepoint & 0x3Fu));
    } else if (codepoint <= 0xFFFFu) {
        encoded[count++] = static_cast<char>(0xE0u | (codepoint >> 12));
        encoded[count++] = static_cast<char>(0x80u | ((codepoint >> 6) & 0x3Fu));
        encoded[count++] = static_cast<char>(0x80u | (codepoint & 0x3Fu));
    } else {
        encoded[count++] = static_cast<char>(0xF0u | (codepoint >> 18));
        encoded[count++] = static_cast<char>(0x80u | ((codepoint >> 12) & 0x3Fu));
        encoded[count++] = static_cast<char>(0x80u | ((codepoint >> 6) & 0x3Fu));
        encoded[count++] = static_cast<char>(0x80u | (codepoint & 0x3Fu));
    }
    if (count == 1 && !charAllowed(encoded[0])) return;
    const std::size_t cap = m_maxBytes - 1;
    if (m_text.size() + count > cap) return;
    if (m_bInsert) {
        m_text.insert(m_caret, encoded, count);
        m_caret += count;
    } else if (m_caret < m_text.size()) {
        const auto end = nextCodepointEnd();
        if (end - m_caret < count && m_text.size() + count - (end - m_caret) > cap) return;
        m_text.replace(m_caret, end - m_caret, encoded, count);
        m_caret += count;
    } else {
        m_text.append(encoded, count);
        m_caret += count;
    }
    fireChange();
}

std::size_t cEditBox::previousCodepointStart() const noexcept {
    if (m_caret == 0) return 0;
    std::size_t pos = m_caret - 1;
    while (pos > 0 && is_utf8_continuation(static_cast<unsigned char>(m_text[pos]))) --pos;
    return pos;
}

std::size_t cEditBox::nextCodepointEnd() const noexcept {
    if (m_caret >= m_text.size()) return m_text.size();
    std::size_t pos = m_caret + 1;
    while (pos < m_text.size() && is_utf8_continuation(static_cast<unsigned char>(m_text[pos]))) ++pos;
    return pos;
}

void cEditBox::deleteAtCaret() {
    // Backspace: delete the char to the left of the caret.
    if (m_caret == 0 || m_text.empty()) return;
    const auto start = previousCodepointStart();
    m_text.erase(start, m_caret - start);
    m_caret = start;
    fireChange();
}

void cEditBox::deleteForwardAtCaret() {
    // Delete: delete the char to the right of the caret.
    if (m_caret >= m_text.size()) return;
    m_text.erase(m_caret, nextCodepointEnd() - m_caret);
    fireChange();
}

bool cEditBox::charAllowed(char c) const noexcept {
    // Treat the char as ASCII for validation purposes. Multi-byte UTF-8
    // characters pass through by default (the byte is non-ASCII so the
    // standard ctype predicates return false, which we want).
    switch (m_validCheck) {
        case 0: return true;
        case 1: return std::isdigit(static_cast<unsigned char>(c)) != 0;
        case 2: return std::isalpha(static_cast<unsigned char>(c)) != 0;
        case 3: return std::isalnum(static_cast<unsigned char>(c)) != 0;
        default: return true;
    }
}

void cEditBox::fireChange() {
    m_bTextChanged = 1;
    if (m_onChange) m_onChange(*this, m_userdata);
}

std::uint32_t cEditBox::ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                                     std::uint32_t /*mouseFlags*/) {
    if (!isEnabled()) return static_cast<std::uint32_t>(WindowEvent::Null);
    const bool inside = PtInWindow(mouseX, mouseY);
    SetFocus(inside);
    if (inside) {
        SetCaret(true);
        return static_cast<std::uint32_t>(WindowEvent::LButtonClick);
    }
    return static_cast<std::uint32_t>(WindowEvent::Null);
}

std::uint32_t cEditBox::ActionKeyboardEvent(std::int32_t key, std::int32_t ch) {
    if (!isEnabled() || !hasFocus()) {
        return static_cast<std::uint32_t>(WindowEvent::Null);
    }
    if (m_bReadOnly) {
        // Read-only: navigation keys still work, character input is rejected.
        // (We do allow Esc / Enter to dismiss even read-only, matching the
        //  "still focused but immutable" use case in the legacy engine.)
    }

    switch (static_cast<Key>(key)) {
        case Key::Back:
            if (!m_bReadOnly) deleteAtCaret();
            return static_cast<std::uint32_t>(WindowEvent::KeyDown);
        case Key::Delete:
            if (!m_bReadOnly) deleteForwardAtCaret();
            return static_cast<std::uint32_t>(WindowEvent::KeyDown);
        case Key::Left:
            m_caret = previousCodepointStart();
            return static_cast<std::uint32_t>(WindowEvent::KeyDown);
        case Key::Right:
            m_caret = nextCodepointEnd();
            return static_cast<std::uint32_t>(WindowEvent::KeyDown);
        case Key::Home:
            m_caret = 0;
            return static_cast<std::uint32_t>(WindowEvent::KeyDown);
        case Key::End:
            m_caret = m_text.size();
            return static_cast<std::uint32_t>(WindowEvent::KeyDown);
        case Key::Enter:
            if (m_onEnter) m_onEnter(*this, m_userdata);
            return static_cast<std::uint32_t>(WindowEvent::KeyDown);
        case Key::Escape:
            if (m_onEscape) m_onEscape(*this, m_userdata);
            return static_cast<std::uint32_t>(WindowEvent::KeyDown);
        case Key::Tab:
            // Legacy engine moves focus to the next control. Skipped here.
            return static_cast<std::uint32_t>(WindowEvent::KeyDown);
        default:
            break;
    }

    // Character input (Char).
    if (ch > 0) {
        if (!m_bReadOnly) insertCodepointAtCaret(static_cast<std::uint32_t>(ch));
        return static_cast<std::uint32_t>(WindowEvent::Char_);
    }
    return static_cast<std::uint32_t>(WindowEvent::Null);
}

} // namespace mxh::ui
