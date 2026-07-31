// cobjectguagen.hpp — modern port of 墨香
// CObjectGuagen (object gauge with effect-time
// interpolation: cGuagen subclass + effect state).
//
// 1:1 port of legacy `CObjectGuagen` from
//   `墨香【源码】\[Client]MH\ObjectGuagen.h` (1596 B)
//   and `墨香【源码】\[Client]MH\ObjectGuagen.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_GUAGENE; init
//     m_fGuageEffectPieceWidth=0, m_fIncAmount=0,
//     m_dwEffectTime=0, m_dwStartTime=0,
//     m_fOldPercentRate=0, m_fCurPercentRate=0,
//     m_bBlink=FALSE, m_dwStartBlinkTime=0,
//     m_fGuageEffectPieceHeightScaleY=1.
//   - Dtor: empty body.
//   - SetValue(GUAGEVAL val, DWORD estTime):
//     clamp val to 1; commented out blink anim; if
//     m_fCurPercentRate > m_fOldPercentRate (energy
//     dropped, blink anim legacy); set
//     m_fOldPercentRate=m_fCurPercentRate; set
//     m_dwEffectTime=estTime; if estTime != 0,
//     m_fIncAmount = (val - m_fOldPercentRate) / estTime
//     + m_dwStartTime = gCurTime; else
//     m_fCurPercentRate = val; cGuagen::SetValue(val).
//   - ActionEvent: cGuagen::ActionEvent + (commented
//     out anim ShakeProcess); return WE_NULL.
//   - Render: cGuagen::Render + effect-time interp
//     + scaleRate + RGBA_MERGE + (commented out blink).
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_type = WT_GUAGENE
//     drop, modern cWindow does not have m_type).
//   - Dtor: empty (no-op).
//   - SetValue: REAL — clamp val + interp state
//     (m_fOldPercentRate / m_fIncAmount / m_dwEffectTime).
//     gCurTime + cAnimationManager (commented out in
//     legacy) are TODO.
//   - ActionEvent: TODO (CMouse not ported, R-12.x
//     deferred). Modern port returns WE_NULL.
//   - Render: TODO (VECTOR2 + cImage::RenderSprite
//     + RGBA_MERGE not ported, R-12.x deferred).
//     Modern port is no-op (calls base cGuagen::Render).
//   - 1:1 quirk: legacy m_dwImageRGB is exposed by modern cWindow.
//     Alpha and option-alpha fields remain deferred because the modern
//     renderer does not yet expose the legacy merge path.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is a Tier 1.5 subcontrol (cGuagen subclass
// prerequisite for CProgressBarDlg and 3 progress
// bar dialogs). The dialog has 1 cImage
// (m_GuageEffectPieceImage) + 6 state fields
// (m_fGuageEffectPieceWidth / m_fIncAmount /
// m_dwEffectTime / m_dwStartTime / m_fOldPercentRate /
// m_fCurPercentRate) + 2 blink flags. gCurTime +
// CMouse + cImage::RenderSprite + cAnimationManager
// are R-12.x deferred.

#pragma once

#include "cguagen.hpp"

#include <cstdint>

namespace mxh::ui {

// 1:1 with legacy GUAGEVAL (typedef float).
using GUAGEVAL = float;

class cObjectGuagen : public cGuagen {
public:
    cObjectGuagen();
    ~cObjectGuagen() override;

    // ----- 1:1 with legacy CObjectGuagen::SetValue -----

    // 1:1 with legacy SetValue(GUAGEVAL val, DWORD estTime).
    // Clamp val to 1; if estTime != 0, set
    // m_fIncAmount = (val - m_fOldPercentRate) / estTime.
    // gCurTime + cAnimationManager (commented out in
    // legacy) are TODO. The blink animation hook
    // (m_bBlink = TRUE; m_dwStartBlinkTime = gCurTime)
    // is preserved as documented field, not
    // activated.
    void SetValue(GUAGEVAL val, std::uint32_t estTime);

    // ----- 1:1 with legacy CObjectGuagen::ActionEvent -----

    // 1:1 with legacy ActionEvent. The whole method
    // is TODO (CMouse not ported, R-12.x deferred).
    // Modern port returns WE_NULL.
    std::uint32_t ActionEvent() noexcept;

    // ----- 1:1 with legacy CObjectGuagen::Render -----

    // 1:1 with legacy Render. The effect-time
    // interpolation + RGBA_MERGE render is TODO
    // (VECTOR2 + cImage::RenderSprite + RGBA_MERGE
    // not ported, R-12.x deferred). Modern port is
    // a no-op (calls base cGuagen::Render).
    void Render() override;

    // ----- 1:1 with legacy CObjectGuagen state accessors -----

    // 1:1 with legacy m_fOldPercentRate / m_fCurPercentRate
    // (used by tests to verify SetValue behavior).
    float GetOldPercentRate() const noexcept { return m_fOldPercentRate; }
    float GetCurPercentRate() const noexcept { return m_fCurPercentRate; }

    // 1:1 with legacy m_fIncAmount / m_dwEffectTime
    // (used by tests to verify SetValue behavior).
    float GetIncAmount() const noexcept   { return m_fIncAmount; }
    std::uint32_t GetEffectTime() const noexcept { return m_dwEffectTime; }

    // 1:1 with legacy m_dwStartTime.
    std::uint32_t GetStartTime() const noexcept { return m_dwStartTime; }

    // 1:1 with legacy m_fGuageEffectPieceWidth /
    // m_fGuageEffectPieceHeightScaleY (set by
    // SetEffectPieceImage / SetGuageEffectPieceWidth
    // / SetGuageEffectPieceHeightScale — these
    // setters preserved 1:1 with legacy).
    float GetGuageEffectPieceWidth() const noexcept {
        return m_fGuageEffectPieceWidth;
    }
    float GetGuageEffectPieceHeightScaleY() const noexcept {
        return m_fGuageEffectPieceHeightScaleY;
    }

    void SetGuageEffectPieceWidth(float width) noexcept {
        m_fGuageEffectPieceWidth = width;
    }
    void SetGuageEffectPieceHeightScale(float hei) noexcept {
        m_fGuageEffectPieceHeightScaleY = hei;
    }

    // ----- 1:1 with legacy blink flag -----

    // 1:1 with legacy m_bBlink (BOOL, init FALSE).
    // Modern port: bool (default member init = false).
    bool IsBlink() const noexcept { return m_bBlink; }
    void SetBlink(bool v) noexcept { m_bBlink = v; }

private:
    // 1:1 with legacy m_GuageEffectPieceImage
    // (cImage). Modern port: cImage is ported but
    // we don't dereference it during testing. The
    // field is inherited from cGuagen::m_GuagePieceImage
    // — but legacy CObjectGuagen has a SEPARATE
    // m_GuageEffectPieceImage field for the "effect
    // overlay" (with scaleRate + RGBA_MERGE). Modern
    // port stores the effect piece image separately
    // (even if unused in Phase 12.x).
    cImage m_GuageEffectPieceImage;

    // 1:1 with legacy state fields. m_dwImageRGB is provided
    // by modern cWindow; m_alpha and m_dwOptionAlpha remain
    // deferred until the legacy RGBA merge render path is ported.
    float m_fGuageEffectPieceWidth = 0.0f;
    float m_fIncAmount = 0.0f;
    std::uint32_t m_dwEffectTime = 0;
    std::uint32_t m_dwStartTime = 0;
    float m_fOldPercentRate = 0.0f;
    float m_fCurPercentRate = 0.0f;
    bool m_bBlink = false;
    std::uint32_t m_dwStartBlinkTime = 0;
    float m_fGuageEffectPieceHeightScaleY = 1.0f;
};

}  // namespace mxh::ui
