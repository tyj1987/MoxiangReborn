// cStatic.cpp — modern implementation of 墨香 cStatic (label).

#include "cStatic.hpp"

#include "TextRender.hpp"

#include <cstdio>
#include <cstdlib>
#include <limits>

namespace mxh::ui {

namespace {

bool checked_add(std::int32_t a, std::int32_t b, std::int32_t& out) {
    const auto sum = static_cast<std::int64_t>(a) + b;
    if (sum < std::numeric_limits<std::int32_t>::min() ||
        sum > std::numeric_limits<std::int32_t>::max()) return false;
    out = static_cast<std::int32_t>(sum);
    return true;
}

bool valid_text_horizontal(std::int32_t x, std::int32_t width,
                           std::int32_t inset) {
    const auto left = static_cast<std::int64_t>(x) + inset;
    const auto edge = static_cast<std::int64_t>(x) + width;
    const auto right = static_cast<std::int64_t>(x) + width - inset;
    return left >= std::numeric_limits<std::int32_t>::min() &&
           left <= std::numeric_limits<std::int32_t>::max() &&
           edge >= std::numeric_limits<std::int32_t>::min() &&
           edge <= std::numeric_limits<std::int32_t>::max() &&
           right >= std::numeric_limits<std::int32_t>::min() &&
           right <= std::numeric_limits<std::int32_t>::max();
}

} // namespace

cStatic::cStatic() = default;
cStatic::~cStatic() = default;

void cStatic::Render() {
    if (!isVisible()) return;
    cWindow::Render();
    if (m_text.empty()) return;

    TextRenderRequest request;
    request.text = m_text;
    request.x = absX();
    if (!checked_add(absY(), m_textY, request.y)) return;
    // The host text adapter combines x, width and text insets for left/right
    // alignment. Reject impossible coordinates before handing it an int32.
    if (!valid_text_horizontal(request.x, width(), m_textX)) return;
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
        if (!checked_add(shadow.x, m_shadowX, shadow.x) ||
            !checked_add(shadow.y, m_shadowY, shadow.y)) return;
        if (!valid_text_horizontal(shadow.x, width(), m_textX)) return;
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
