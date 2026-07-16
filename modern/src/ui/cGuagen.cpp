// cGuagen.cpp — modern implementation of 墨香 cGuagen (progress bar).

#include "cGuagen.hpp"

#include <algorithm>

namespace mxh::ui {

cGuagen::cGuagen() = default;

cGuagen::~cGuagen() = default;

void cGuagen::Render() {
    // No-op: actual draw goes through the 6.4+ cImage seam
    // (mxh::ui::cImage::render) which the Phase 5 renderer integration
    // hooks up. This skeleton just stores the geometry; see the
    // cImage test for the full render path example.
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
