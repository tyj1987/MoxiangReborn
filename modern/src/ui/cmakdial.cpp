// cmakdial.cpp — 1:1 port of 墨香 cCharMakeDlg (character
// creation dialog). See cmakdial.hpp for the data-model
// rationale + 1:1 quirks.

#include "cmakdial.hpp"
#include "cstatic.hpp"
#include "legacy_window_event.hpp"

namespace mxh::ui {

cCharMakeDlg::cCharMakeDlg() = default;

cCharMakeDlg::~cCharMakeDlg() = default;

void cCharMakeDlg::Linking() {
    // 1:1 with legacy cCharMakeDlg::Linking(). The legacy
    // originally used cComboBoxEx* for the 4 selectors; the
    // 2008-era code was downgraded to cStatic* (the
    // combo-boxes were replaced with left/right arrow
    // buttons that rotate the selection in the manager).
    // The modern port mirrors the downgraded form.
    //
    // The id assignment is (matches the modern test convention;
    // the legacy CMID_* ids live in WindowIDEnum.h, not yet
    // ported):
    //   200  ManHairType
    //   201  WomanHairType
    //   202  ManFaceType
    //   203  WomanFaceType
    auto resolve = [this](std::int32_t id) -> cStatic* {
        return static_cast<cStatic*>(findWindowById(id));
    };
    m_pMHair  = resolve(200);
    m_pWMHair = resolve(201);
    m_pMFace  = resolve(202);
    m_pWMFace = resolve(203);
}

void cCharMakeDlg::ChangeComboStatus(std::uint16_t wSex) {
    // 1:1 with legacy cCharMakeDlg::ChangeComboStatus(WORD
    // wSex). 0 = male (M hair + M face visible, W hidden),
    // 1 = female (the opposite). The legacy calls
    // m_pMHair->SetActive(TRUE) on the cStatic children,
    // but the legacy cStatic class does not actually
    // define SetActive (the legacy code did not compile
    // cleanly — see KNOWN_BUGS / R-12 follow-up). The
    // modern port uses SetVisible (1:1 with the show/hide
    // intent) on cWindow, which cStatic inherits. This
    // matches the spirit of the legacy behavior (show the
    // active selector, hide the other) without depending
    // on a method that the legacy cStatic doesn't actually
    // have.
    if (wSex == 0) {
        if (m_pMHair)  m_pMHair->SetVisible(true);
        if (m_pWMHair) m_pWMHair->SetVisible(false);
        if (m_pMFace)  m_pMFace->SetVisible(true);
        if (m_pWMFace) m_pWMFace->SetVisible(false);
    } else {
        if (m_pMHair)  m_pMHair->SetVisible(false);
        if (m_pWMHair) m_pWMHair->SetVisible(true);
        if (m_pMFace)  m_pMFace->SetVisible(false);
        if (m_pWMFace) m_pWMFace->SetVisible(true);
    }
}

void cCharMakeDlg::OnActionEvent(std::int32_t lId, void* /*p*/, std::uint32_t we) {
    // 1:1 with legacy cCharMakeDlg::OnActionEvent. The legacy
    // dispatches WE_BTNCLICK to CHARMAKEMGR->RotateSelection
    // for each button id. The modern port routes through an
    // injected RotateCallback (the host wires it to
    // CHARMAKEMGR->RotateSelection in the future singleton
    // bridge). Until the host injects a callback, OnActionEvent
    // is effectively a no-op (matches the pre-bridge state).
    if ((we & legacy_window_event::kButtonClick) == 0) return;

    switch (lId) {
    case kSexLeftId:
        Rotate(CharMakeCategory::Sex, kCharMakePrev);
        break;
    case kSexRightId:
        Rotate(CharMakeCategory::Sex, kCharMakeNext);
        break;
    case kHairLeftId:
        if (m_pMHair && m_pMHair->isVisible())
            Rotate(CharMakeCategory::MHair, kCharMakePrev);
        else if (m_pWMHair && m_pWMHair->isVisible())
            Rotate(CharMakeCategory::WMHair, kCharMakePrev);
        break;
    case kHairRightId:
        if (m_pMHair && m_pMHair->isVisible())
            Rotate(CharMakeCategory::MHair, kCharMakeNext);
        else if (m_pWMHair && m_pWMHair->isVisible())
            Rotate(CharMakeCategory::WMHair, kCharMakeNext);
        break;
    case kFaceLeftId:
        if (m_pMHair && m_pMHair->isVisible())
            Rotate(CharMakeCategory::MFace, kCharMakePrev);
        else if (m_pWMHair && m_pWMHair->isVisible())
            Rotate(CharMakeCategory::WMFace, kCharMakePrev);
        break;
    case kFaceRightId:
        if (m_pMHair && m_pMHair->isVisible())
            Rotate(CharMakeCategory::MFace, kCharMakeNext);
        else if (m_pWMHair && m_pWMHair->isVisible())
            Rotate(CharMakeCategory::WMFace, kCharMakeNext);
        break;
    case kClothLeftId:
        Rotate(CharMakeCategory::Wear, kCharMakePrev);
        break;
    case kClothRightId:
        Rotate(CharMakeCategory::Wear, kCharMakeNext);
        break;
    case kBootLeftId:
        Rotate(CharMakeCategory::Boot, kCharMakePrev);
        break;
    case kBootRightId:
        Rotate(CharMakeCategory::Boot, kCharMakeNext);
        break;
    case kWeaponLeftId:
        Rotate(CharMakeCategory::Weapon, kCharMakePrev);
        break;
    case kWeaponRightId:
        Rotate(CharMakeCategory::Weapon, kCharMakeNext);
        break;
    case kAttribLeftId:
        if (m_attribEnabled)
            Rotate(CharMakeCategory::Attribute, kCharMakePrev);
        break;
    case kAttribRightId:
        if (m_attribEnabled)
            Rotate(CharMakeCategory::Attribute, kCharMakeNext);
        break;
    default:
        break;
    }
}

void cCharMakeDlg::Rotate(CharMakeCategory cat, std::int32_t dir) noexcept {
    // 1:1 with legacy CHARMAKEMGR->RotateSelection dispatch.
    // The modern port defers the actual rotation logic to the
    // host (which calls into the future CharMakeManager
    // singleton bridge); the dialog itself only forwards the
    // request.
    if (m_rotate) {
        m_rotate(cat, dir);
    }
}

}  // namespace mxh::ui
