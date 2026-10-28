// mugongsuryundialog.hpp — modern port of 墨香 CMugongSuryunDialog.
//
// 1:1 port of legacy `CMugongSuryunDialog` from
//   `墨香【源码】\[Client]MH\MugongSuryunDialog.{h,cpp}`.
//
// CMugongSuryunDialog is a cTabDialog subclass that hosts the
// mugong (skills) and suryun (practice) tabs. It dispatches
// Add() based on legacy WT_* type, captures the inner
// CMugongDialog + CSuryunDialog references for the wrapper
// to call, and overrides SetActive to dismiss the
// MBI_MUGONGDELETE msgbox on dialog close.
//
// 1:1 contract preserved:
//   - Add(cWindow* window) — 2-phase dispatch: first captures
//     the inner CMugongDialog (m_pMugongDlg) and CSuryunDialog
//     (m_pSuryunDlg); then routes to AddTabBtn / AddTabSheet
//     / cDialog::Add based on legacy WT_* type. Modern port:
//     cWindow has no m_type/GetType() (Phase 6 removed), so
//     dispatch uses dynamic_cast (cPushupButton → tab btn,
//     CMugongDialog OR CSuryunDialog → tab sheet, else →
//     base Add).
//   - SetActive(BOOL val) override — on val==FALSE: dismiss
//     the MBI_MUGONGDELETE msgbox via WINDOWMGR (stubbed),
//     then SetDisable(FALSE) on this dialog, then
//     cTabDialog::SetActive(val). On val==TRUE: just
//     cTabDialog::SetActive(TRUE).
//   - OnActionEvent — empty body in legacy. Modern port
//     preserves the empty body verbatim (1:1 quirk).
//   - FakeMoveIcon(x, y, icon) — 1:1 wrap: forwards to
//     m_pMugongDlg->FakeMoveIcon(x, y, icon). Modern port
//     preserves the wrap + defensive null guard (legacy
//     would have crashed if m_pMugongDlg was null).
//
// 1:1 quirks preserved:
//   - 1:1 quirk: legacy ctor sets `m_type = WT_MUGONGSURYUNDIALOG`.
//     Modern port drops m_type entirely (Phase 6 removed,
//     same as cTipBrowserDlg/cPetStateMiniDlg/cSkillPointNotify).
//   - 1:1 quirk: legacy ctor initialises `m_pMugongDlg = NULL;
//     m_pSuryunDlg = NULL;`. Modern port uses
//     `std::unique_ptr` default-init (nullptr).
//   - 1:1 quirk: legacy `Add` has a TYPO bug:
//       if (window->GetType() == WT_MUGONGDIALOG)
//           m_pMugongDlg = (CMugongDialog*)window;
//       else if (window->GetType() == WT_MUGONGDIALOG)  // BUG: should be WT_SURYUNDIALOG
//           m_pSuryunDlg = (CSuryunDialog*)window;
//     The second if checks the same `WT_MUGONGDIALOG`, so
//     m_pSuryunDlg is **always NULL** in legacy. Modern
//     port: 1:1 preserve this bug — m_pSuryunDlg never gets
//     set, regardless of input. Test confirms nullptr.
//   - 1:1 quirk: legacy `OnActionEvent` has an empty body
//     (no return statement). Modern port preserves the
//     empty body verbatim. Same pattern as cLoadingDlg
//     (0.13.31).
//   - 1:1 quirk: legacy `SetActive(BOOL val)` calls
//     `cTabDialog::SetActive(val)` AFTER its own
//     `SetDisable(FALSE)` and msgbox-dismissal. Modern
//     port: same order — base SetActive is called last.
//   - 1:1 quirk: legacy `SetActive(FALSE)` calls
//     `SetDisable(FALSE)` on itself. This is a self-undo:
//     if the dialog is disabled, val==FALSE un-disables
//     it (so the close logic can run). Modern port:
//     preserves this self-undo (same pattern as cDialog's
//     SetDisable/SetActive interplay).
//   - 1:1 quirk: legacy SetActive has a commented-out
//     `CMainBarDialog* pDlg = GAMEIN->GetMainInterfaceDialog()`
//     block that would push the OPT_MUGONGDLGICON. Modern
//     port: documented as 1:1 quirk, not ported (GAMEIN
//     not ported + render wiring deferred).
//   - 1:1 quirk: legacy SetActive's first line is
//     `if(!val)`, which means the msgbox-dismissal +
//     SetDisable block only runs on val==FALSE. Modern
//     port preserves this exact conditional.
//   - 1:1 quirk: legacy `FakeMoveIcon` returns
//     `m_pMugongDlg->FakeMoveIcon(x,y,icon)` directly.
//     If m_pMugongDlg is null (legacy bug scenario), this
//     would crash (UB). Modern port: defensive null guard
//     returning false (1:1 quirk documented).
//   - 1:1 quirk: legacy `Add` uses legacy WT_PUSHUPBUTTON
//     type check for AddTabBtn. Modern port uses
//     `dynamic_cast<cPushupButton*>` (same end-state).
//   - 1:1 quirk: legacy `Add` uses legacy
//     `WT_MUGONGDIALOG || WT_SURYUNDIALOG` for AddTabSheet.
//     Modern port uses
//     `dynamic_cast<CMugongDialog*> || dynamic_cast<CSuryunDialog*>`.
//   - 1:1 quirk: legacy 4-singleton dispatch (GAMEIN/
//     WINDOWMGR/CMainBarDialog/CMugongDialog/CSuryunDialog)
//     — all stubbed no-op per Phase 6 pattern.
//   - 1:1 quirk: legacy `m_pMugongDlg` / `m_pSuryunDlg` are
//     raw pointers (legacy owns the dialogs but only
//     references them). Modern port: `std::unique_ptr`
//     default-init (caller transfers ownership via Add).
//   - 1:1 quirk: legacy MBI_MUGONGDELETE comes from
//     WindowIDEnum.h. Modern port: local
//     kMbiMugongDelete=2300 to avoid coupling to the
//     legacy header.
//   - 1:1 quirk: legacy destructor is empty (no cleanup).
//     Modern port: `= default` (unique_ptr auto-cleans).

#pragma once

#include "ctabdialog.hpp"

#include <cstdint>
#include <memory>

namespace mxh::ui {

class cWindow;
class cIcon;
class cPushupButton;
class cDialog;

// 1:1 stubs: legacy `CMugongDialog` + `CSuryunDialog` (defined
// in [Client]MH/MugongDialog.h + SuryunDialog.h, part of the
// 2008-era client). Modern port uses opaque forward-declared
// classes with the only API surface MugongSuryunDialog needs
// (FakeMoveIcon for CMugongDialog; constructor for both). The
// dialog also uses the base cDialog for ForTesting access.
// Production code can replace these stubs by linking the real
// legacy headers.
class CMugongDialog : public cDialog {
public:
    CMugongDialog() = default;
    // 1:1 quirk: legacy doesn't override Init; modern port
    // also doesn't (uses cDialog's Init).
    bool FakeMoveIconForTesting(std::int32_t x, std::int32_t y, cIcon* icon) {
        ++s_fakeMoveIconCalls;
        (void)x; (void)y; (void)icon;
        return false;
    }
    static std::uint32_t fakeMoveIconCallCount() noexcept { return s_fakeMoveIconCalls; }
    static void ClearFakeMoveIconCallCount() noexcept { s_fakeMoveIconCalls = 0; }
private:
    static inline std::uint32_t s_fakeMoveIconCalls = 0;
};

class CSuryunDialog : public cDialog {
public:
    CSuryunDialog() = default;
};

// 1:1 with legacy WindowIDEnum.h. Local id range to avoid
// coupling to the legacy header.
constexpr int kMbiMugongDelete = 2300;

class cMugongSuryunDialog : public cTabDialog {
public:
    cMugongSuryunDialog();
    ~cMugongSuryunDialog() override;

    cMugongSuryunDialog(const cMugongSuryunDialog&) = delete;
    cMugongSuryunDialog& operator=(const cMugongSuryunDialog&) = delete;

    // 1:1 with legacy surface.
    void Add(cWindow* window);  // 1:1 quirk: legacy `virtual void Add(cWindow*)`
                                // override. Modern cWindow::Add is non-virtual;
                                // modern port uses public `Add` (same as
                                // cMallNoticeDialog 0.13.68).
    void SetActive(bool val) noexcept override;
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);
    bool FakeMoveIcon(std::int32_t x, std::int32_t y, cIcon* icon);

    // 1:1 with legacy accessors.
    CSuryunDialog* GetSuryunDialog()  const noexcept { return m_pSuryunDlg; }
    CMugongDialog* GetMugongDialog()  const noexcept { return m_pMugongDlg; }

    // Test accessors.
    std::uint32_t msgboxDismissCount() const noexcept { return s_msgboxDismissCount; }
    std::uint32_t setDisableFalseCount() const noexcept { return s_setDisableFalseCount; }
    std::uint32_t fakeMoveIconCallCount() const noexcept { return s_fakeMoveIconCalls; }
    std::uint32_t onActionEventCallCount() const noexcept { return s_onActionEventCalls; }
    std::uint32_t addCallCount() const noexcept { return s_addCalls; }

    // Test-injectable: WINDOWMGR msgbox state.
    static void SetMsgboxPresentForTesting(bool present) noexcept {
        s_msgboxPresent = present;
    }
    static bool msgboxPresentForTesting() noexcept { return s_msgboxPresent; }
    static void ClearTestInjections() noexcept;

private:
    // 1:1 quirk: legacy m_pMugongDlg / m_pSuryunDlg are raw
    // pointers (caller-owned, the tab container only
    // references them). Modern port: same — raw pointers.
    // The `cWindow` ownership goes through cTabDialog's
    // m_ppWindowTabSheet (unique_ptr), so the same window
    // can be both referenced by m_pMugongDlg raw ptr AND
    // owned by m_ppWindowTabSheet unique_ptr.
    CMugongDialog* m_pMugongDlg = nullptr;
    CSuryunDialog* m_pSuryunDlg = nullptr;

    // Test-injectable state (replace the legacy globals:
    // WINDOWMGR, MBI_MUGONGDELETE msgbox, GAMEIN).
    static inline std::uint32_t s_msgboxDismissCount = 0;
    static inline std::uint32_t s_setDisableFalseCount = 0;
    static inline std::uint32_t s_fakeMoveIconCalls = 0;
    static inline std::uint32_t s_onActionEventCalls = 0;
    static inline std::uint32_t s_addCalls = 0;
    static inline bool s_msgboxPresent = false;
};

} // namespace mxh::ui
