// cobjectguagen.cpp — 1:1 port of 墨香
// CObjectGuagen (object gauge with effect-time
// interpolation). See cobjectguagen.hpp for the
// data-model rationale + 1:1 quirks.

#include "cobjectguagen.hpp"

namespace mxh::ui {

cObjectGuagen::cObjectGuagen() {
    // 1:1 with legacy CObjectGuagen ctor:
    //   m_type = WT_GUAGENE;
    //   m_fGuageEffectPieceWidth = 0.f;
    //   m_fIncAmount = 0.f;
    //   m_dwEffectTime = 0;
    //   m_dwStartTime = 0;
    //   m_fOldPercentRate = 0.f;
    //   m_fCurPercentRate = 0.f;
    //   m_bBlink = FALSE;
    //   m_dwStartBlinkTime = 0;
    //   m_fGuageEffectPieceHeightScaleY = 1.f;
    //
    // 1:1 quirk: modern cWindow does not have
    // m_type field (removed in Phase 6 when cWindow
    // was modernized). The ctor body is dropped.
    // All other fields use default member init
    // (= 0 / 0.0f / false / 1.0f).
}

cObjectGuagen::~cObjectGuagen() = default;

void cObjectGuagen::SetCurrentTimeProvider(
    GetCurrentTimeFn getCurrentTime, void* userData) noexcept {
    m_getCurrentTimeFn = getCurrentTime;
    m_clockUserData = userData;
}

void cObjectGuagen::SetValue(GUAGEVAL val, std::uint32_t estTime) {
    // 1:1 with legacy CObjectGuagen::SetValue. The
    // legacy is:
    //   if (val > 1) val = 1;
    //   if (m_fCurPercentRate > m_fOldPercentRate) {
    //     // m_bBlink = TRUE;  // commented out
    //     // m_dwStartBlinkTime = gCurTime;  // commented out
    //   }
    //   m_fOldPercentRate = m_fCurPercentRate;
    //   m_dwEffectTime = estTime;
    //   if (m_dwEffectTime) {
    //     m_fIncAmount = (val - m_fOldPercentRate) / m_dwEffectTime;
    //     m_dwStartTime = gCurTime;  // TODO: gCurTime not ported
    //   } else {
    //     m_fCurPercentRate = val;
    //   }
    //   cGuagen::SetValue(val);
    //
    // The modern port:
    //   - Clamp val to 1.
    //   - m_bBlink activation is documented in the
    //     header but not implemented (1:1 with
    //     legacy commented-out code).
    //   - gCurTime not ported — m_dwStartTime is
    //     left at 0 (the value is documented but
    //     not used in tests).
    //   - cGuagen::SetValue(val) is REAL (base class).
    if (val > 1.0f) {
        val = 1.0f;
    }
    // 1:1 with legacy commented-out blink anim.
    // m_bBlink = (m_fCurPercentRate > m_fOldPercentRate);
    // m_dwStartBlinkTime = gCurTime;  // TODO

    m_fOldPercentRate = m_fCurPercentRate;
    m_dwEffectTime = estTime;
    if (m_dwEffectTime) {
        m_fIncAmount = (val - m_fOldPercentRate) /
                        static_cast<float>(m_dwEffectTime);
        // Legacy stamps the effect with gCurTime. The host provider
        // supplies the same DWORD millisecond value when wired.
        m_dwStartTime = m_getCurrentTimeFn
            ? m_getCurrentTimeFn(m_clockUserData)
            : 0u;
    } else {
        m_fCurPercentRate = val;
    }
    cGuagen::SetValue(val);
}

std::uint32_t cObjectGuagen::ActionEvent() noexcept {
    // 1:1 with legacy CObjectGuagen::ActionEvent.
    // The legacy is:
    //   DWORD we = cGuagen::ActionEvent(mouseInfo);
    //   // if (m_bActive) m_ani.ShakeProcess();
    //   return WE_NULL;
    //
    // The modern port: the whole method is TODO
    // (CMouse not ported, R-12.x deferred). Modern
    // port returns WE_NULL.
    // TODO: CMouse not ported (R-12.x deferred).
    //       When ported, the body becomes the legacy
    //       code with the cGuagen::ActionEvent +
    //       commented-out anim ShakeProcess.
    return 0;  // WE_NULL
}

void cObjectGuagen::Render() {
    cGuagen::Render();

    // Preserve the legacy effect-time interpolation state even though
    // the modern renderer does not yet draw the overlay sprite.
    if (!isActive() || !m_dwEffectTime || !m_getCurrentTimeFn) {
        return;
    }
    const std::uint32_t currentTime =
        m_getCurrentTimeFn(m_clockUserData);
    const std::uint32_t elapsedTime = currentTime - m_dwStartTime;
    if (elapsedTime < m_dwEffectTime) {
        m_fCurPercentRate = m_fOldPercentRate
            + static_cast<float>(elapsedTime) * m_fIncAmount;
        if (m_fCurPercentRate > 1.0f) {
            m_fCurPercentRate = 1.0f;
        }
        return;
    }

    m_fCurPercentRate = GetValue();
    m_dwEffectTime = 0;
    m_dwStartTime = 0;
}

}  // namespace mxh::ui
