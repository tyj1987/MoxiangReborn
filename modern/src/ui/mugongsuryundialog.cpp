// mugongsuryundialog.cpp — modern port of 墨香 CMugongSuryunDialog.
//
// 1:1 port body. See legacy `MugongSuryunDialog.cpp` for the original.

#include "mugongsuryundialog.hpp"

#include "cicondialog.hpp"
#include "cpushupbutton.hpp"
#include "cwindow.hpp"
#include "cdialog.hpp"
#include "cmsgbox.hpp"

#include <cstdint>
#include <memory>

namespace mxh::ui {

cMugongSuryunDialog::cMugongSuryunDialog() = default;
cMugongSuryunDialog::~cMugongSuryunDialog() = default;

void cMugongSuryunDialog::ClearTestInjections() noexcept {
    s_msgboxDismissCount = 0;
    s_setDisableFalseCount = 0;
    s_fakeMoveIconCalls = 0;
    s_onActionEventCalls = 0;
    s_addCalls = 0;
    s_msgboxPresent = false;
    CMugongDialog::ClearFakeMoveIconCallCount();
}

void cMugongSuryunDialog::Add(cWindow* window) {
    ++s_addCalls;
    if (!window) { return; }

    // 1:1 with legacy Add(cWindow* window):
    //   if (window->GetType() == WT_MUGONGDIALOG)
    //       m_pMugongDlg = (CMugongDialog*)window;
    //   else if (window->GetType() == WT_MUGONGDIALOG)  // BUG: should be WT_SURYUNDIALOG
    //       m_pSuryunDlg = (CSuryunDialog*)window;
    //
    //   if (window->GetType() == WT_PUSHUPBUTTON)
    //       AddTabBtn(curIdx1++, (cPushupButton*)window);
    //   else if (window->GetType() == WT_MUGONGDIALOG ||
    //            window->GetType() == WT_SURYUNDIALOG)
    //   {
    //       AddTabSheet(curIdx2++, window);
    //   }
    //   else
    //       cDialog::Add(window);
    //
    // 1:1 quirks:
    //   - legacy 1:1 BUG: the second `if` in the first pair
    //     compares against `WT_MUGONGDIALOG` (not
    //     `WT_SURYUNDIALOG`). This means m_pSuryunDlg is
    //     never set, regardless of input. Modern port:
    //     1:1 preserves this bug — m_pSuryunDlg stays
    //     nullptr for the dialog's lifetime.
    //   - legacy cWindow::GetType() returns WT_* (from
    //     m_type). Modern cWindow has no m_type (Phase 6
    //     removed). Modern port: dispatch uses
    //     `dynamic_cast` for the 3-way branch.
    //   - legacy curIdx1/curIdx2 are inherited from
    //     cTabDialog. Modern port: uses cTabDialog's
    //     curIdx1_/curIdx2_ (0.13.68 fix: protected).
    auto* asMugongDlg = dynamic_cast<CMugongDialog*>(window);
    auto* asSuryunDlg = dynamic_cast<CSuryunDialog*>(window);
    if (asMugongDlg) {
        // 1:1 with legacy: m_pMugongDlg = (CMugongDialog*)window
        m_pMugongDlg = asMugongDlg;
    }
    // 1:1 quirk: legacy second if checks WT_MUGONGDIALOG
    // again (bug). Modern port: preserve the bug — use the
    // same dynamic_cast check that matches WT_MUGONGDIALOG
    // (i.e. CMugongDialog). The legacy cast
    // `(CSuryunDialog*)window` is never reached because
    // the if-condition always fails (only CMugongDialog
    // matches `WT_MUGONGDIALOG`). Modern port: same —
    // m_pSuryunDlg stays nullptr for the dialog's lifetime.
    if (asMugongDlg) {  // 1:1 quirk: same check (not asSuryunDlg)
        // 1:1 quirk: in legacy, this would assign
        // `m_pSuryunDlg = (CSuryunDialog*)window` — but
        // since the if-condition is `WT_MUGONGDIALOG`, the
        // window is actually a CMugongDialog, not a
        // CSuryunDialog. The cast `(CSuryunDialog*)window`
        // is technically wrong but legacy never entered
        // this branch (the prior if already captured
        // CMugongDialog). Modern port: preserve the legacy
        // behavior — do NOT enter this branch. Documented
        // as 1:1 quirk (the cast would have been UB if
        // entered).
    }

    if (auto* btn = dynamic_cast<cPushupButton*>(window)) {
        AddTabBtn(curIdx1_++, std::unique_ptr<cPushupButton>(btn));
    } else if (asMugongDlg || asSuryunDlg) {
        // 1:1 quirk: legacy uses `WT_MUGONGDIALOG ||
        // WT_SURYUNDIALOG`. Modern port uses
        // `dynamic_cast<CMugongDialog*> ||
        // dynamic_cast<CSuryunDialog*>`.
        //
        // 1:1 quirk: legacy stores the window as a tab
        // sheet AND increments curIdx2. Modern port: same.
        // m_pMugongDlg raw pointer references the same
        // window (not owned), and m_ppWindowTabSheet
        // unique_ptr owns it.
        AddTabSheet(curIdx2_++, std::unique_ptr<cWindow>(window));
    } else {
        cDialog::Add(std::unique_ptr<cWindow>(window));
    }
}

void cMugongSuryunDialog::OnActionEvent(std::int32_t /*lId*/, void* /*p*/,
                                        std::uint32_t /*we*/) {
    // 1:1 quirk: legacy OnActionEvent body is empty (no
    // implementation, no return statement). Modern port
    // preserves the empty body verbatim. Same pattern as
    // cLoadingDlg 0.13.31.
    ++s_onActionEventCalls;
}

bool cMugongSuryunDialog::FakeMoveIcon(std::int32_t x, std::int32_t y,
                                       cIcon* icon) {
    // 1:1 with legacy FakeMoveIcon:
    //   return m_pMugongDlg->FakeMoveIcon(x, y, icon);
    //
    // 1:1 quirks:
    //   - legacy returns the inner FakeMoveIcon's result
    //     directly. Modern port: same.
    //   - legacy UB: if m_pMugongDlg is null (legacy bug
    //     + tests can hit this), legacy would crash.
    //     Modern port: defensive null guard returning
    //     false (1:1 quirk documented).
    ++s_fakeMoveIconCalls;
    if (!m_pMugongDlg) {
        return false;
    }
    return m_pMugongDlg->FakeMoveIconForTesting(x, y, icon);
}

void cMugongSuryunDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy SetActive(BOOL val):
    //   if (!val) {
    //       cMsgBox* pMsgBox = (cMsgBox*)WINDOWMGR->GetWindowForID(MBI_MUGONGDELETE);
    //       if (pMsgBox)
    //           WINDOWMGR->AddListDestroyWindow(pMsgBox);
    //       SetDisable(FALSE);
    //   }
    //   cTabDialog::SetActive(val);
    //
    // 1:1 quirks:
    //   - legacy WINDOWMGR + MBI_MUGONGDELETE stubbed no-op
    //     per Phase 6 pattern. Modern port: msgbox state
    //     is test-injectable via s_msgboxPresent.
    //   - legacy `SetDisable(FALSE)` is a self-undo: if the
    //     dialog is currently disabled, val==FALSE
    //     un-disables it so the close logic can run.
    //     Modern port: same.
    //   - legacy cTabDialog::SetActive(val) is called LAST
    //     (after the msgbox-dismissal + self-undo). Modern
    //     port: same order.
    if (!val) {
        // 1:1 quirk: legacy WINDOWMGR->GetWindowForID +
        // AddListDestroyWindow. Modern port: msgbox
        // presence is test-injectable; if present,
        // record the dismiss.
        if (s_msgboxPresent) {
            ++s_msgboxDismissCount;
        }
        // 1:1 quirk: legacy SetDisable(FALSE) on self.
        // Modern port: same (cDialog::SetDisable is
        // virtual noexcept override).
        SetDisable(false);
        ++s_setDisableFalseCount;
    }
    cTabDialog::SetActive(val);
}

} // namespace mxh::ui
