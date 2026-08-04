// titanpartsprogressbardlg.cpp — 1:1 port of 墨香
// CTitanPartsProgressBarDlg (titan-parts make progress
// bar dialog). See titanpartsprogressbardlg.hpp for
// the data-model rationale + 1:1 quirks.

#include "titanpartsprogressbardlg.hpp"
#include "cobjectguagen.hpp"
#include "cstatic.hpp"

namespace mxh::ui {

cTitanPartsProgressBarDlg::cTitanPartsProgressBarDlg() {
    // 1:1 with legacy CTitanPartsProgressBarDlg ctor:
    //   empty body.
}

cTitanPartsProgressBarDlg::~cTitanPartsProgressBarDlg() = default;

void cTitanPartsProgressBarDlg::Linking() {
    // 1:1 with legacy CTitanPartsProgressBarDlg::Linking.
    SetProgressGuagen(static_cast<cObjectGuagen*>(
        findWindowById(kIdProgressBarGage)));
    SetRemaintimeStatic(static_cast<cStatic*>(
        findWindowById(kIdRemaintimeTime)));
}

void cTitanPartsProgressBarDlg::SetCancelCallback(
    ReEnableFn reEnable, void* userData) noexcept {
    m_reEnableFn = reEnable;
    m_reEnableUserData = userData;
}

void cTitanPartsProgressBarDlg::OnActionEvent(std::int32_t lId, void* p,
                                              std::uint32_t we) {
    // 1:1 with legacy CTitanPartsProgressBarDlg::OnActionEvent.
    // The legacy is:
    //   switch (lId) {
    //   case TITANPARTS_PROGRESSBAR_CANCEL:
    //     InitProgress();
    //     GAMEIN->GetTitanPartsMakeDlg()->SetDisable(FALSE);
    //     break;
    //   }
    //
    // InitProgress() remains REAL (resets state + SetActive(false)).
    // GAMEIN->GetTitanPartsMakeDlg()->SetDisable(FALSE) is replaced
    // by an optional host-injected re-enable callback so the host
    // controls the parent dialog lifecycle.
    (void)p;
    (void)we;
    if (lId == kIdCancelBtn) {
        InitProgress();
        if (m_reEnableFn) {
            m_reEnableFn(m_reEnableUserData);
        }
    }
}

}  // namespace mxh::ui
