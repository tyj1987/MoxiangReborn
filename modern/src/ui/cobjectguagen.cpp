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
        // TODO: 1:1 with legacy m_dwStartTime = gCurTime;
        //       gCurTime not ported (R-12.x deferred).
        m_dwStartTime = 0;
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
    // 1:1 with legacy CObjectGuagen::Render. The
    // legacy is:
    //   cGuagen::Render();
    //   if (m_bActive) {
    //     VECTOR2 imgPosRect = ...;
    //     VECTOR2 scaleRate;
    //     float per = m_fPercentRate;
    //     if (m_dwEffectTime) {
    //       if (gCurTime - m_dwStartTime < m_dwEffectTime) {
    //         m_fCurPercentRate = ...;
    //         per = m_fCurPercentRate;
    //         if (per > 1.0) per = 1.0;
    //       } else {
    //         m_fCurPercentRate = per;
    //         m_dwEffectTime = 0;
    //         m_dwStartTime = 0;
    //       }
    //     }
    //     scaleRate.x = m_fGuageWidth * per / m_fGuageEffectPieceWidth;
    //     scaleRate.y = m_fGuageEffectPieceHeightScaleY;
    //     // (commented out blink + RGBA_MERGE render)
    //   }
    //
    // The modern port: the effect-time interp +
    // VECTOR2 + RGBA_MERGE render is TODO (R-12.x
    // deferred). Modern port calls base cGuagen::Render.
    cGuagen::Render();
    // TODO: VECTOR2 + cImage::RenderSprite + RGBA_MERGE
    //       + gCurTime not ported (R-12.x deferred).
    //       When ported, the body becomes the legacy
    //       code with the effect-time interp + RGBA_MERGE
    //       render.
}

}  // namespace mxh::ui
