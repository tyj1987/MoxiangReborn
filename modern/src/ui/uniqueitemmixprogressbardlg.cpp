// uniqueitemmixprogressbardlg.cpp — 1:1 port of
// 墨香 CUniqueItemMixProgressBarDlg (unique-item
// mix progress bar dialog). See
// uniqueitemmixprogressbardlg.hpp for the data-model
// rationale + 1:1 quirks.

#include "uniqueitemmixprogressbardlg.hpp"
#include "cobjectguagen.hpp"
#include "cstatic.hpp"

namespace mxh::ui {

cUniqueItemMixProgressBarDlg::cUniqueItemMixProgressBarDlg() {
    // 1:1 with legacy CUniqueItemMixProgressBarDlg ctor:
    //   empty body.
}

cUniqueItemMixProgressBarDlg::~cUniqueItemMixProgressBarDlg() = default;

void cUniqueItemMixProgressBarDlg::Linking() {
    // 1:1 with legacy CUniqueItemMixProgressBarDlg::Linking.
    SetProgressGuagen(static_cast<cObjectGuagen*>(
        findWindowById(kIdProgressBarGage)));
    SetRemaintimeStatic(static_cast<cStatic*>(
        findWindowById(kIdRemaintimeTime)));
}

void cUniqueItemMixProgressBarDlg::SetCancelCallback(
    ReEnableFn reEnable, void* userData) noexcept {
    m_reEnableFn = reEnable;
    m_reEnableUserData = userData;
}

void cUniqueItemMixProgressBarDlg::OnActionEvent(std::int32_t lId, void* p,
                                                std::uint32_t we) {
    // 1:1 with legacy CUniqueItemMixProgressBarDlg::OnActionEvent.
    // The legacy is:
    //   switch (lId) {
    //   case UNIQUEITEMMIX_PROGRESSBAR_CANCEL:
    //     InitProgress();
    //     GAMEIN->GetUniqueItemMixDlg()->SetDisable(FALSE);
    //     break;
    //   }
    //
    // InitProgress() remains REAL (resets state + SetActive(false)).
    // GAMEIN->GetUniqueItemMixDlg()->SetDisable(FALSE) is replaced
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
