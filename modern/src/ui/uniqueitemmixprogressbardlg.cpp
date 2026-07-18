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

void cUniqueItemMixProgressBarDlg::OnActionEvent(std::int32_t lId, void* p,
                                                std::uint32_t we) {
    (void)p;
    (void)we;
    if (lId == kIdCancelBtn) {
        InitProgress();
        // TODO: GAMEIN not ported (R-12.x deferred).
    }
}

}  // namespace mxh::ui
