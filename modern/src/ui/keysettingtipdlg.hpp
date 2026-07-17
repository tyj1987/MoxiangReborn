// keysettingtipdlg.hpp — modern port of 墨香 CKeySettingTipDlg
// (keyboard shortcut tip dialog: 2 cImageSelf + 1 cDialog
// Render).
//
// 1:1 port of legacy `CKeySettingTipDlg` from
//   `墨香【源码】\[Client]MH\KeySettingTipDlg.h` (331 B) and
//   `墨香【源码】\[Client]MH\KeySettingTipDlg.cpp`.
//
// What the legacy does:
//   - Ctor: m_wMode = 2 (default to "hidden" mode).
//   - Dtor: empty body.
//   - Linking: 2 cImageSelf slots, each loads a
//     sprite from disk (Image/2D/KeySetting1.tga
//     and Image/2D/KeySetting2.tga) and sets the
//     source rect to (0,0,1024,768).
//   - SetMode(WORD): inline setter, m_wMode = mode.
//   - Render: if !m_bActive, return. If m_wMode > 1,
//     return. Otherwise call cDialog::RenderWindow,
//     then m_KeyImage[m_wMode].RenderSprite(...)
//     with VECTOR2 scale (1, 768/1024), color white,
//     and m_absPos; then call
//     cDialog::RenderComponent.
//
// The modern port covers:
//   - Ctor: m_wMode = 2 (1:1 with legacy default).
//   - Dtor: empty (no-op).
//   - Linking: TODO (cImageSelf not ported, R-12.x
//     deferred). Modern port stores the image
//     paths as std::string so the data model is
//     1:1 (the legacy's cImageSelf::LoadSprite is
//     the only operation, and the modern port
//     documents the call as TODO).
//   - SetMode: REAL — inline setter.
//   - Render: no-op (1:1 with cTextArea::Render
//     pattern; the actual sprite rendering needs
//     cImageSelf port). 1:1 quirk: the modern
//     Render returns immediately (the legacy
//     guards `if(!m_bActive) return; if(m_wMode
//     > 1) return;` are documented in the
//     TODO).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 21st **Tier 2** dialog port (after
// cIntroReplayDlg). The dialog has no service
// dependency on the modern service interface
// (Phase 13) — only cImageSelf + VECTOR2 (R-12.x
// deferred).

#pragma once

#include "cdialog.hpp"

#include <cstdint>
#include <string>

namespace mxh::ui {

class cKeySettingTipDlg : public cDialog {
public:
    cKeySettingTipDlg();
    ~cKeySettingTipDlg() override;

    // ----- 1:1 with legacy CKeySettingTipDlg::Linking -----

    // 1:1 with legacy Linking. Calls
    // cImageSelf::LoadSprite + SetImageSrcRect on
    // 2 slots. Modern port: TODO (cImageSelf not
    // ported, R-12.x deferred). Stores the
    // resource paths as std::string for the data
    // model (1:1 with legacy .tga path strings).
    void Linking();

    // ----- 1:1 with legacy CKeySettingTipDlg::SetMode -----

    // 1:1 with legacy SetMode (inline setter).
    void SetMode(std::uint16_t mode) noexcept { m_wMode = mode; }

    // ----- 1:1 with legacy CKeySettingTipDlg::GetMode -----

    std::uint16_t GetMode() const noexcept { return m_wMode; }

    // ----- 1:1 with legacy CKeySettingTipDlg::Render override -----

    // 1:1 with legacy Render. The legacy guards
    // `if(!m_bActive) return; if(m_wMode > 1)
    // return;` are documented; the actual sprite
    // rendering is TODO (cImageSelf + VECTOR2 not
    // ported, R-12.x deferred). Modern port is a
    // no-op (same pattern as cTextArea::Render).
    void Render() override {}

    // ----- 1:1 constants -----

    // kModeHidden: legacy m_wMode = 2 default. The
    // guard `if(m_wMode > 1) return;` in Render
    // means modes 0 and 1 show image 0/1; mode
    // 2 (and above) hides.
    static constexpr std::uint16_t kModeHidden = 2;

    // kNumImages: 2 cImageSelf slots (1:1 with
    // legacy m_KeyImage[2]).
    static constexpr std::size_t kNumImages = 2;

private:
    // 1:1 with legacy m_KeyImage[2] (2 cImageSelf
    // slots). Modern port stores resource paths
    // as std::string instead of cImageSelf (which
    // is forward-declared and not ported).
    std::string m_imagePaths[kNumImages];

    // 1:1 with legacy m_wMode. Default 2 (= hidden).
    std::uint16_t m_wMode = kModeHidden;
};

}  // namespace mxh::ui
