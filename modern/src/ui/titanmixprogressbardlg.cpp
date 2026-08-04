// titanmixprogressbardlg.cpp — 1:1 port of 墨香
// CTitanMixProgressBarDlg (titan-mix progress bar
// dialog). See titanmixprogressbardlg.hpp for the
// data-model rationale + 1:1 quirks.

#include "titanmixprogressbardlg.hpp"
#include "cobjectguagen.hpp"
#include "cstatic.hpp"

namespace mxh::ui {

cTitanMixProgressBarDlg::cTitanMixProgressBarDlg() {
    // 1:1 with legacy CTitanMixProgressBarDlg ctor:
    //   empty body.
    //
    // 1:1 quirk: ctor body is empty. No init
    // required (all fields use default member init
    // from base cProgressBarDlg).
}

cTitanMixProgressBarDlg::~cTitanMixProgressBarDlg() = default;

void cTitanMixProgressBarDlg::Linking() {
    // 1:1 with legacy CTitanMixProgressBarDlg::Linking.
    // The legacy is:
    //   m_pProgressGuagen = (CObjectGuagen*)GetWindowForID(TITANMIX_PROGRESSBAR_GAGE);
    //   m_pRemaintimeStatic = (cStatic*)GetWindowForID(TITANMIX_PROGRESSBAR_TIME);
    //
    // The modern port: resolve 2 children by id +
    // call base setters (the base class's fields
    // are protected via setters since cProgressBarDlg
    // stores m_pProgressGuagen as private — wait,
    // they're private; use the public setters).
    SetProgressGuagen(static_cast<cObjectGuagen*>(
        findWindowById(kIdProgressBarGage)));
    SetRemaintimeStatic(static_cast<cStatic*>(
        findWindowById(kIdRemaintimeTime)));
}

void cTitanMixProgressBarDlg::SetCancelCallback(
    ReEnableFn reEnable, void* userData) noexcept {
    m_reEnableFn = reEnable;
    m_reEnableUserData = userData;
}

void cTitanMixProgressBarDlg::OnActionEvent(std::int32_t lId, void* p,
                                            std::uint32_t we) {
    // 1:1 with legacy CTitanMixProgressBarDlg::OnActionEvent.
    // The legacy is:
    //   switch (lId) {
    //   case TITANMIX_PROGRESSBAR_CANCEL:
    //     InitProgress();
    //     GAMEIN->GetTitanMixDlg()->SetDisable(FALSE);
    //     break;
    //   }
    //
    // InitProgress is REAL (calls base cProgressBarDlg::InitProgress
    // which resets state + SetActive(false)). The
    // GAMEIN->GetTitanMixDlg()->SetDisable(FALSE) call is replaced
    // by an optional host-injected re-enable callback wired through
    // SetCancelCallback.
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
