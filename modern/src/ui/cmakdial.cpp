// cmakdial.cpp — 1:1 port of 墨香 cCharMakeDlg (character
// creation dialog). See cmakdial.hpp for the data-model
// rationale + 1:1 quirks.

#include "cmakdial.hpp"
#include "cstatic.hpp"

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

void cCharMakeDlg::OnActionEvent(std::int32_t /*lId*/, void* /*p*/,
                                 std::uint32_t /*we*/) {
    // TODO: dispatch to CharMakeManager once that singleton
    // is ported. The legacy calls
    //   CHARMAKEMGR->RotateSelection(CE_SEX,        CM_PREV/NEXT)
    //   CHARMAKEMGR->RotateSelection(CE_MHAIR,      CM_PREV/NEXT)
    //   CHARMAKEMGR->RotateSelection(CE_WMHAIR,     CM_PREV/NEXT)
    //   CHARMAKEMGR->RotateSelection(CE_MFACE,      CM_PREV/NEXT)
    //   CHARMAKEMGR->RotateSelection(CE_WMFACE,     CM_PREV/NEXT)
    //   CHARMAKEMGR->RotateSelection(CE_WEAR,       CM_PREV/NEXT)
    //   CHARMAKEMGR->RotateSelection(CE_BOOT,       CM_PREV/NEXT)
    //   CHARMAKEMGR->RotateSelection(CE_WEAPON,     CM_PREV/NEXT)
    // for 8 button ids (CMID_SexLeft/Right, CMID_HairLeft/Right,
    // CMID_FaceLeft/Right, CMID_ClothLeft/Right,
    // CMID_BootLeft/Right, CMID_WeaponLeft/Right). The
    // CharMakeManager port is tracked separately; until
    // then, OnActionEvent is a no-op (the dialog's UI state
    // can still be observed through Linking +
    // ChangeComboStatus, which is the testable surface).
    //
    // When CharMakeManager is ported, the implementation
    // will be:
    //   if (we & WE_BTNCLICK) {
    //       if      (lId == CMID_SexLeft)        rotate(CE_SEX,    CM_PREV);
    //       else if (lId == CMID_SexRight)       rotate(CE_SEX,    CM_NEXT);
    //       else if (lId == CMID_HairLeft  && m_pMHair ->IsActive()) rotate(CE_MHAIR,  CM_PREV);
    //       ... etc.
    //   }
}

}  // namespace mxh::ui
