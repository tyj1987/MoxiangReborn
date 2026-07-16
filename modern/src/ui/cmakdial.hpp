// cmakdial.hpp — modern port of 墨香 cCharMakeDlg (character
// creation dialog: sex / hair / face / cloth / boot / weapon
// selectors).
//
// 1:1 port of legacy `cCharMakeDlg` from
//   `墨香【源码】\[Client]MH\CharMakeDialog.h` (659 B) and
//   `墨香【源码】\[Client]MH\CharMakeDialog.cpp` (1,689 B).
//
// The dialog has two pieces of state:
//   - 4 cStatic pointers (m/f hair + m/f face) for visibility
//     toggling when the player changes sex (1:1 with the
//     legacy's commented-out cComboBoxEx form, which was
//     downgraded to cStatic in 2008-era legacy code).
//   - An OnActionEvent handler that dispatches left/right
//     button clicks to the global CharMakeManager singleton
//     via `CHARMAKEMGR->RotateSelection(CE_*, CM_PREV/NEXT)`.
//
// The modern port covers the dialog structure (Linking +
// ChangeComboStatus + OnActionEvent) but does NOT port
// CharMakeManager itself — the singleton manages the
// per-character creation state (hair index, face index,
// etc.) and requires a separate port. Until then,
// OnActionEvent's CHARMAKEMGR->RotateSelection calls are
// commented out (with a clear TODO referencing the
// pending CharMakeManager port). The dialog itself
// remains testable: Linking + ChangeComboStatus (sex
// toggle) are observable without the manager.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this
// is the second **Tier 2** dialog port (after
// cMacroDialog in 0.13.14). The dialog has no service
// dependency on the modern service interface (Phase 13)
// — all state is local UI state. The CharMakeManager port
// is tracked as a future Tier 1.5 work item.

#pragma once

#include "cDialog.hpp"

namespace mxh::ui {

class cStatic;

class cCharMakeDlg : public cDialog {
public:
    cCharMakeDlg();
    ~cCharMakeDlg() override;

    // ----- 1:1 with legacy cCharMakeDlg::Linking -----

    // Resolves the 4 cStatic children (m/f hair + m/f face)
    // by id. The legacy uses CMID_ManHairType /
    // CMID_WomanHairType / CMID_ManFaceType /
    // CMID_WomanFaceType from WindowIDEnum.h; the modern
    // port uses a local id range (200-203) until the full
    // WindowIDEnum is ported (see cListDialogEx for the
    // same pattern).
    void Linking();

    // ----- 1:1 with legacy cCharMakeDlg::ChangeComboStatus -----

    // Toggle the visible cStatic based on the player's sex.
    // 0 = male (M hair + M face active, W hidden),
    // 1 = female (W hair + W face active, M hidden).
    void ChangeComboStatus(std::uint16_t wSex);

    // ----- 1:1 with legacy cCharMakeDlg::OnActionEvent -----

    // Dispatch a button click. The legacy calls
    // CHARMAKEMGR->RotateSelection(CE_*, CM_PREV/NEXT) for
    // each id. The modern port defers the manager call
    // until the CharMakeManager singleton is ported; see
    // the TODO in cmakdial.cpp.
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- Accessors (used by tests + future CharMakeManager bridge) -----

    cStatic* GetManHair()   const noexcept { return m_pMHair; }
    cStatic* GetWomanHair() const noexcept { return m_pWMHair; }
    cStatic* GetManFace()   const noexcept { return m_pMFace; }
    cStatic* GetWomanFace() const noexcept { return m_pWMFace; }

private:
    cStatic* m_pMHair  = nullptr;
    cStatic* m_pWMHair = nullptr;
    cStatic* m_pMFace  = nullptr;
    cStatic* m_pWMFace = nullptr;
};

}  // namespace mxh::ui
