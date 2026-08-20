// cStatic.cpp — modern implementation of 墨香 cStatic (label).

#include "cStatic.hpp"

#include "TextRender.hpp"

#include <cstdio>
#include <cstdlib>

namespace mxh::ui {

cStatic::cStatic() = default;
cStatic::~cStatic() = default;

void cStatic::Render() {
    if (!isVisible()) return;
    cWindow::Render();
    if (m_text.empty()) return;

    TextRenderRequest request;
    request.text = m_text;
    request.x = absX();
    request.y = absY() + m_textY;
    request.width = width();
    request.height = height();
    request.left_inset = m_textX;
    request.right_inset = m_textX;
    request.color = m_fgColor;
    request.font_index = m_fontIdx;
    request.align = static_cast<TextRenderAlign>(m_align);
    request.multiline = m_multiLine;

    if (m_shadow) {
        auto shadow = request;
        shadow.x += m_shadowX;
        shadow.y += m_shadowY;
        shadow.color = m_shadowColor;
        renderText(shadow);
    }
    renderText(request);
}

void cStatic::SetStaticText(std::string text) {
    m_text = std::move(text);
}

void cStatic::SetStaticValue(std::int32_t v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", v);
    m_text = buf;
}

std::int32_t cStatic::GetStaticValue() const noexcept {
    if (m_text.empty()) return 0;
    return std::atoi(m_text.c_str());
}

} // namespace mxh::ui
