// cGuagen.cpp — modern implementation of 墨香 cGuagen (progress bar).

#include "cGuagen.hpp"

#include <algorithm>

namespace mxh::ui {

cGuagen::cGuagen() = default;

cGuagen::~cGuagen() = default;

void cGuagen::Render() {
    if (m_GuagePieceImage.IsNull() || !isVisible()) return;
    const float fullWidth = m_fGuageWidth > 0.0f
        ? m_fGuageWidth : static_cast<float>(width());
    const float targetHeight = static_cast<float>(height()) *
                               m_fGuagePieceHeightScaleY;
    const auto targetWidth = static_cast<std::int32_t>(fullWidth * m_fPercentRate);
    if (targetWidth <= 0 || targetHeight <= 0.0f) return;
    (void)m_GuagePieceImage.render(
        absX() + static_cast<std::int32_t>(m_imgRelPos.x),
        absY() + static_cast<std::int32_t>(m_imgRelPos.y),
        targetWidth, static_cast<std::int32_t>(targetHeight));
}

void cGuagen::SetValue(float val) {
    m_fPercentRate = val;
    // Legacy semantics: clamp to [0, 1].
    if (m_fPercentRate > 1.0f) {
        m_fPercentRate = 1.0f;
    }
    // Negative values are not clamped in legacy either; the ctor
    // zero-initializes m_fPercentRate and callers set non-negative
    // values. Mirror the legacy "1.f < m_fPercentRate" branch only.
}

void cGuagen::SetGuageImagePos(std::int32_t imgX, std::int32_t imgY) {
    m_imgRelPos.x = static_cast<float>(imgX);
    m_imgRelPos.y = static_cast<float>(imgY);
}

void cGuagen::SetGuageImagePos(float imgX, float imgY) {
    m_imgRelPos.x = imgX;
    m_imgRelPos.y = imgY;
}

}  // namespace mxh::ui
