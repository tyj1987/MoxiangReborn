// ctextarea.cpp — 1:1 port of 墨香 cTextArea (multi-line
// text area). See ctextarea.hpp for the data-model
// rationale + 1:1 quirks.

#include "ctextarea.hpp"

#include <algorithm>
#include <cstring>

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
            m_scriptText.erase(m_caretPos - 1, 1);
            --m_caretPos;
        }
        return static_cast<std::uint32_t>(WindowEvent::KeyDown);
    }
    if (key == 37) { if (m_caretPos > 0) --m_caretPos; return static_cast<std::uint32_t>(WindowEvent::KeyDown); }
    if (key == 39) { if (m_caretPos < m_scriptText.size()) ++m_caretPos; return static_cast<std::uint32_t>(WindowEvent::KeyDown); }
    if (key == 36) { m_caretPos = 0; return static_cast<std::uint32_t>(WindowEvent::KeyDown); }
    if (key == 35) { m_caretPos = m_scriptText.size(); return static_cast<std::uint32_t>(WindowEvent::KeyDown); }
    if (key == 46) {
        if (!m_bReadOnly && m_caretPos < m_scriptText.size()) m_scriptText.erase(m_caretPos, 1);
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
    if (ch > 0 && ch < 0x80) {
        if (!m_bReadOnly && (m_nMaxLine <= 0 ||
                             static_cast<int>(m_scriptText.size()) < m_nMaxLine)) {
            m_scriptText.insert(m_caretPos, 1, static_cast<char>(ch));
            ++m_caretPos;
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

}  // namespace mxh::ui
