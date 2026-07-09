// mxh/ui/cEditBox.cpp
// Phase 6.2 — implementation of the modern cEditBox widget.
#include "cEditBox.hpp"

#include <cctype>

namespace mxh::ui {

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
    m_text  = std::move(text);
    m_caret = m_text.size();
    fireChange();
}

std::string cEditBox::displayText() const {
    if (!m_bSecret) return m_text;
    return std::string(m_text.size(), '*');
}

void cEditBox::SetCaretPos(std::size_t pos) noexcept {
    if (pos > m_text.size()) pos = m_text.size();
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

void cEditBox::deleteAtCaret() {
    // Backspace: delete the char to the left of the caret.
    if (m_caret == 0 || m_text.empty()) return;
    m_text.erase(m_text.begin() + static_cast<std::ptrdiff_t>(m_caret) - 1);
    --m_caret;
    fireChange();
}

void cEditBox::deleteForwardAtCaret() {
    // Delete: delete the char to the right of the caret.
    if (m_caret >= m_text.size()) return;
    m_text.erase(m_text.begin() + static_cast<std::ptrdiff_t>(m_caret));
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
            if (m_caret > 0) --m_caret;
            return static_cast<std::uint32_t>(WindowEvent::KeyDown);
        case Key::Right:
            if (m_caret < m_text.size()) ++m_caret;
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
    if (ch > 0 && ch < 0x80) {
        if (!m_bReadOnly) insertCharAtCaret(static_cast<char>(ch));
        return static_cast<std::uint32_t>(WindowEvent::Char_);
    }
    return static_cast<std::uint32_t>(WindowEvent::Null);
}

} // namespace mxh::ui
