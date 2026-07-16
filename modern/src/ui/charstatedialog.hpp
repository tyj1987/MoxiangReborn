// charstatedialog.hpp — modern port of 墨香 CCharStateDialog
// (character state bar: PK / Move / KyungGong / PeaceWar / Ungi
// mode buttons).
//
// 1:1 port of legacy `CCharStateDialog` from
//   `墨香【源码】\[Client]MH\CharStateDialog.h` (701 B) and
//   `墨香【源码】\[Client]MH\CharStateDialog.cpp`.
//
// What the legacy does:
//   - Linking() resolves 5 cPushupButton children (PK /
//     Move / KyungGong / PeaceWar / Ungi) by id and
//     SetPassive(TRUE) on each (so user can't toggle them
//     directly — the code alone flips their state via
//     SetXxxMode).
//   - 5 SetXxxMode methods (SetPKMode / SetMoveMode /
//     SetKyungGongMode / SetPeaceWarMode / SetUngiMode)
//     each call m_pBtnXxx->SetPush(bMode). 1:1 quirk:
//     SetPeaceWarMode inverts the argument (`SetPush(!bPeace)`)
//     because the legacy stores PeaceWar as the OPPOSITE
//     of the underlying "peace" flag.
//   - OnActionEvent dispatches 5 button ids (CSS_BTN_*)
//     to MACROMGR->PlayMacro(ME_TOGGLE_*) for Move /
//     PeaceWar + PKMGR->ToggleHeroPKMode() for PK.
//     KyungGong + Ungi branches are commented out in
//     the legacy (1:1 quirk: the macros were not
//     implemented in the legacy engine).
//   - Refresh() rebuilds tooltips on 3 of the 5 buttons
//     using SCRIPTMGR / RESRCMGR / MACROMGR / GAMEIN
//     singletons. KyungGong + Ungi tooltips are
//     commented out (matches OnActionEvent pattern).
//
// The modern port covers everything that doesn't need
// a singleton: Linking (real), 5 SetXxxMode methods
// (real — pure widget state). OnActionEvent is a no-op
// (4-singleton dispatch deferred). Refresh is a no-op
// (4-singleton tooltip rebuild deferred).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 5th **Tier 2** dialog port (after
// cExitDialog, cMacroDialog, cCharMakeDlg,
// cGuildJoinDialog). The dialog has no service
// dependency on the modern service interface (Phase 13)
// for the widget-state surface; the singleton
// dependencies (MACROMGR / PKMGR / SCRIPTMGR / RESRCMGR /
// GAMEIN) are tracked as future Tier 3+ work items
// (MACROMGR depends on MacroManager service, which is
// not yet ported; PKMGR depends on PlayerStatsService,
// already in Phase 13.2).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cPushupButton;

class cCharStateDialog : public cDialog {
public:
    cCharStateDialog();
    ~cCharStateDialog() override;

    // ----- 1:1 with legacy CCharStateDialog::Linking -----

    // Resolves 5 cPushupButton children (PK / Move /
    // KyungGong / PeaceWar / Ungi) by id and SetPassive(TRUE)
    // on each. Defensive null-checks (each button is
    // optional — if missing, the Set*Mode method skips
    // the SetPush call). Local id range 220-224
    // (cCharMakeDlg uses 200-203, cGuildJoinDialog uses
    // 210-212, so pick a non-overlapping range).
    void Linking();

    // ----- 1:1 with legacy CCharStateDialog::OnActionEvent -----

    // Dispatch a button click. The legacy handles
    // WE_PUSHUP + WE_PUSHDOWN for 5 button ids. The
    // modern port is a no-op until MACROMGR + PKMGR are
    // ported. See the TODO in charstatedialog.cpp for
    // the exact dispatch logic.
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- 1:1 with legacy 5 SetXxxMode methods (REAL, no singleton) -----

    void SetPKMode(bool bPKMode) noexcept;
    void SetMoveMode(bool bRun) noexcept;
    void SetKyungGongMode(bool bKyungGong) noexcept;
    void SetPeaceWarMode(bool bPeace) noexcept;   // 1:1 quirk: inverts arg
    void SetUngiMode(bool bUngi) noexcept;

    // ----- 1:1 with legacy CCharStateDialog::Refresh -----

    // Rebuild tooltips. The modern port is a no-op
    // until SCRIPTMGR + RESRCMGR + MACROMGR + GAMEIN
    // singletons are ported. See the TODO in
    // charstatedialog.cpp for the exact tooltip
    // rebuild logic.
    void Refresh();

    // ----- Accessors (used by tests + future singleton bridge) -----

    cPushupButton* GetPKBtn()         const noexcept { return m_pBtnPK; }
    cPushupButton* GetMoveBtn()       const noexcept { return m_pBtnMove; }
    cPushupButton* GetKyungGongBtn()  const noexcept { return m_pBtnKyungGong; }
    cPushupButton* GetPeaceWarBtn()   const noexcept { return m_pBtnPeaceWar; }
    cPushupButton* GetUngiBtn()       const noexcept { return m_pBtnUngi; }

    // ----- Local id range (matches modern test convention) -----

    static constexpr std::int32_t kBtnPKId        = 220;  // was CSS_BTN_PK
    static constexpr std::int32_t kBtnMoveId      = 221;  // was CSS_BTN_MOVE
    static constexpr std::int32_t kBtnKyungGongId = 222;  // was CSS_BTN_KYUNGGONG
    static constexpr std::int32_t kBtnPeaceWarId  = 223;  // was CSS_BTN_PEACEWAR
    static constexpr std::int32_t kBtnUngiId      = 224;  // was CSS_BTN_UNGI

private:
    cPushupButton* m_pBtnPK        = nullptr;
    cPushupButton* m_pBtnMove      = nullptr;
    cPushupButton* m_pBtnKyungGong = nullptr;
    cPushupButton* m_pBtnPeaceWar  = nullptr;
    cPushupButton* m_pBtnUngi      = nullptr;
};

}  // namespace mxh::ui
