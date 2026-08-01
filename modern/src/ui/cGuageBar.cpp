#include "mxh/ui/cGuageBar.hpp"

#include "mxh/ui/cButton.hpp"

namespace mxh::ui {

cGuageBar::cGuageBar() {
    // 1:1 with legacy cGuageBar default ctor. m_type = WT_GUAGEBAR
    // quirk is dropped (modern cWindow does not have m_type).
    m_fVertical = false;
    m_barRelPos = 0.0f;
    m_pbarBtn = nullptr;
    m_minValue = 0;
    m_maxValue = 0;
    m_curValue = 0;
    m_interval = 0;
    m_fBarDrag = false;
    m_bLock = false;
}

cGuageBar::~cGuageBar() {
    // 1:1 with legacy SAFE_DELETE(m_pbarBtn). The button is owned
    // by the parent dialog, so we don't delete it here.
    m_pbarBtn = nullptr;
}

void cGuageBar::InitGuageBar(std::int32_t interval, bool vertical) {
    // 1:1 with legacy InitGuageBar(LONG interval, BOOL vertical).
    // Legacy ASSERT(interval > 0); modern port: clamp interval to
    // a minimum of 1 to avoid divide-by-zero in GetCurRate().
    m_interval = interval > 0 ? interval : 1;
    m_fVertical = vertical;
}

void cGuageBar::Add(cWindow* btn) {
    // 1:1 with legacy Add(cWindow * btn). Legacy ASSERTed on second
    // call; modern port silently ignores.
    if (m_pbarBtn != nullptr) {
        return;
    }
    m_pbarBtn = static_cast<cButton*>(btn);
    if (m_pbarBtn == nullptr) {
        return;
    }
    // 1:1 with legacy SetAbsXY positioning of m_pbarBtn at the
    // bar's abs position + button's rel position.
    m_pbarBtn->SetAbsXY(absX() + m_pbarBtn->relX(),
                        absY() + m_pbarBtn->relY());
    m_startPos = m_fVertical ? m_pbarBtn->relY()
                             : m_pbarBtn->relX();
    // 1:1 with legacy m_interval -= (vertical ? btn->GetHeight()
    //                                         : btn->GetWidth());
    const std::int32_t shrink = m_fVertical ? m_pbarBtn->height()
                                            : m_pbarBtn->width();
    m_interval -= shrink;
    if (m_interval < 1) {
        m_interval = 1;
    }
}

void cGuageBar::InitValue(std::int32_t minv, std::int32_t maxv,
                          std::int32_t curv) {
    // 1:1 with legacy InitValue ordering.
    m_minValue = minv;
    m_maxValue = maxv;
    m_curValue = curv;
    repositioning();
}

void cGuageBar::SetMinValue(std::int32_t minv) {
    m_minValue = minv;
    repositioning();
}

void cGuageBar::SetMaxValue(std::int32_t maxv) {
    m_maxValue = maxv;
    repositioning();
}

void cGuageBar::SetCurValue(std::int32_t val) {
    m_curValue = val;
    repositioning();
}

void cGuageBar::SetInterval(std::int32_t val) {
    m_interval = val;
    repositioning();
}

float cGuageBar::GetCurRate() const noexcept {
    // 1:1 with legacy rate = m_barRelPos / (float)m_interval.
    if (m_interval <= 0) {
        return 0.0f;
    }
    return m_barRelPos / static_cast<float>(m_interval);
}

void cGuageBar::SetCurRate(float fRate) noexcept {
    // 1:1 with legacy m_barRelPos = interval * fRate.
    if (m_interval <= 0) {
        m_barRelPos = 0.0f;
        cGuagen::SetValue(0.0f);
        return;
    }
    m_barRelPos = static_cast<float>(m_interval) * fRate;
    // Mirror the rate into cGuagen's piece width so the bar visually
    // matches the logical rate. (cGuagen::SetValue clamps 0..1.)
    cGuagen::SetValue(GetCurRate());
}

void cGuageBar::SetGuageLock(bool bDo, std::uint32_t btnColor) {
    // 1:1 with legacy lock + color. Lock check happens in the
    // future ActionEvent override (CMouse port deferred).
    m_bLock = bDo;
    m_btnColor = btnColor;
    if (m_pbarBtn) {
        m_pbarBtn->SetImageRGB(btnColor);
    }
}

void cGuageBar::SetAbsXY(std::int32_t x, std::int32_t y) noexcept {
    // 1:1 with legacy override. Also reposition m_pbarBtn to track
    // the bar's new abs position.
    cWindow::SetAbsXY(x, y);
    if (m_pbarBtn) {
        m_pbarBtn->SetAbsXY(absX() + m_pbarBtn->relX(),
                            absY() + m_pbarBtn->relY());
    }
}

std::uint32_t cGuageBar::ActionEvent() noexcept {
    // 1:1 with legacy ActionEvent. 1:1 quirk: returns WE_NULL if
    // m_bActive is FALSE. R-12.x: full drag-on-mouse-down handling
    // is deferred (CMouse + cWindowManager integration is TODO).
    if (!isActive()) {
        return 0;
    }
    if (m_bLock) {
        return 0;
    }
    // Mirror the stored rate into cGuagen's display value so any
    // consumer reading the parent class sees the latest rate.
    cGuagen::SetValue(GetCurRate());
    return 0;
}

void cGuageBar::Render() {
    // 1:1 with legacy Render: cWindow::Render + m_pbarBtn->Render().
    // We delegate the bar fill to cGuagen::Render (base class).
    cGuagen::Render();
    if (m_pbarBtn) {
        m_pbarBtn->Render();
    }
}

void cGuageBar::repositioning() {
    // 1:1 with legacy KES 030825 formula:
    //   m_barRelPos = interval * (curValue - minValue) / (maxValue - minValue)
    if (m_maxValue - m_minValue <= 0) {
        return;
    }
    m_barRelPos = static_cast<float>(m_interval)
                * static_cast<float>(m_curValue - m_minValue)
                / static_cast<float>(m_maxValue - m_minValue);
    cGuagen::SetValue(GetCurRate());
}

}  // namespace mxh::ui
