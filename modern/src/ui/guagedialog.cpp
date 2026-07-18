// guagedialog.cpp — modern port of 墨香 CGuageDialog (mussang guage).
//
// 1:1 port body. See legacy `GuageDialog.cpp` for the original.

#include "guagedialog.hpp"

#include "cButton.hpp"
#include "cObjectGuagen.hpp"
#include "cStatic.hpp"

#include <cstdint>
#include <memory>

namespace mxh::ui {

namespace {

// 1:1 stub: legacy HERO + MUSSANGMGR singletons are not ported.
// Modern port: no-op emitter. Host app can wire a real HERO/MUSSANGMGR
// by linking before the dialog lib (per Phase 6 stub pattern).
inline void StubSendMsgMussangOn()  { /* no-op */ }
inline bool StubHeroIsDied()        { return true; }   // conservative: never allow
inline bool StubHeroInTitan()       { return true; }   // conservative: never allow

}  // namespace

cGuageDialog::cGuageDialog() = default;
cGuageDialog::~cGuageDialog() = default;

void cGuageDialog::Linking() {
    // 1:1 with legacy: resolve 3 children by id. Modern port uses
    // std::make_unique<> in-place (跟 cSkillPointNotify / cMoneyDlg /
    // cPetStateMiniDlg pattern 一致). Linking is idempotent (re-call
    // does not re-create the children).
    if (!m_pMussangBtn) {
        auto p = std::make_unique<cButton>();
        p->Init(0, 0, 32, 32, nullptr, nullptr, nullptr, nullptr, nullptr,
                kIdMussangBtn);
        m_pMussangBtn = std::move(p);
    }
    if (!m_pFlicker01) {
        auto p = std::make_unique<cStatic>();
        p->Init(0, 0, 32, 32, nullptr, kIdFlicker01);
        // 1:1 quirk: legacy `m_pFlicker01->SetActive(FALSE);` — but
        // cStatic has no SetActive (cWindow has SetVisible in modern
        // per R-12 fix). Modern port uses SetVisible(false) which is
        // the 1:1 behavioral equivalent.
        p->SetVisible(false);
        m_pFlicker01 = std::move(p);
    }
    // 1:1 quirk: legacy `((CObjectGuagen*)GetWindowForID(CG_GUAGEMUSSANG))->
    // SetValue( 0, 0 );`. Modern port materializes a cObjectGuagen and
    // calls SetValue(0, 0) on it.
    auto* w = findWindowById(kIdGuageMussang);
    if (auto* og = dynamic_cast<cObjectGuagen*>(w)) {
        og->SetValue(0.0f, 0u);
    }
    // 1:1 quirk: legacy `if( m_pMussangBtn ) DisableMussangBtn(TRUE);` —
    // unconditional disable on Link. Modern port matches (no null guard
    // because Linking materialised the button above).
    DisableMussangBtn(true);
}

void cGuageDialog::OnActionEvent(std::int32_t lId, void* /*p*/,
                                  std::uint32_t we) {
    // 1:1 with legacy `if( we & WE_BTNCLICK )` — modern LButtonClick (4)
    // maps to legacy WE_BTNCLICK (64) via static_cast pattern.
    if (we != static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick)) {
        return;
    }
    if (lId == kIdMussangBtn) {
        // 1:1 quirk: legacy `if(!HERO->IsDied() && !HERO->InTitan())
        // MUSSANGMGR->SendMsgMussangOn();` — the mussang button only
        // works when the hero is alive and not in titan form. Stub
        // returns true (conservative: never allow).
        if (StubHeroIsDied() || StubHeroInTitan()) {
            return;
        }
        StubSendMsgMussangOn();
    }
}

void cGuageDialog::Render() {
    // 1:1 with legacy: Render = FlickerMussangGuage + cDialog::Render.
    // Modern port: cDialog::Render is a no-op stub (Phase 6.3 GPU path
    // not ported); FlickerMussangGuage advances the flicker state.
    FlickerMussangGuage();
    cDialog::Render();
}

void cGuageDialog::DisableMussangBtn(bool bDisable) {
    // 1:1 with legacy:
    //   DWORD FullColor = 0xffffffff;
    //   DWORD HalfColor = RGBA_MAKE(200,200,200,255);
    //   if( bDisable ) FullColor = HalfColor;
    //   m_pMussangBtn->SetDisable(bDisable);
    //   m_pMussangBtn->SetImageRGB(FullColor);
    //
    // Modern port: cButton has SetEnabled (no SetDisable in modern
    // Phase 6, per R-12 fix). 1:1 quirk: the image RGB is stored in
    // m_imageRGB for test inspection; the visual layer (R-10 cImage
    // GPU) is not ported.
    constexpr std::uint32_t kFullColor = 0xFFFFFFFFu;
    // 1:1 quirk: legacy `RGBA_MAKE(200,200,200,255)` packs 4 bytes
    // (A, R, G, B = 0xFFC8C8C8) into a DWORD. In the modern port we
    // store the same value (0xFFC8C8C8) as the half-color hint.
    constexpr std::uint32_t kHalfColor = 0xFFC8C8C8u;
    m_imageRGB = bDisable ? kHalfColor : kFullColor;
    if (m_pMussangBtn) {
        m_pMussangBtn->SetEnabled(!bDisable);
    }
}

void cGuageDialog::SetFlicker(bool bFlicker) {
    // 1:1 with legacy: store flag + reset swap time + show/hide overlay.
    m_bFlicker = bFlicker;
    m_dwFlickerSwapTime = m_nowMillis;
    if (m_pFlicker01) {
        m_pFlicker01->SetVisible(bFlicker);
    }
}

void cGuageDialog::FlickerMussangGuage() {
    // 1:1 with legacy: if not flickering, early return; if
    // `gCurTime - m_dwFlickerSwapTime > FLICKER_TIME`, swap visibility
    // and reset swap time. Legacy uses gCurTime (engine global);
    // modern port uses the test-injectable m_nowMillis.
    if (!m_bFlicker) {
        return;
    }
    if (m_nowMillis - m_dwFlickerSwapTime > kFlickerTimeMs) {
        m_bFlActive = !m_bFlActive;
        if (m_pFlicker01) {
            m_pFlicker01->SetVisible(m_bFlActive);
        }
        m_dwFlickerSwapTime = m_nowMillis;
    }
}

} // namespace mxh::ui
