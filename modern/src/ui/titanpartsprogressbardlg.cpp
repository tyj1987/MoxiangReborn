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

void cTitanPartsProgressBarDlg::OnActionEvent(std::int32_t lId, void* p,
                                              std::uint32_t we) {
    // 1:1 with legacy CTitanPartsProgressBarDlg::OnActionEvent.
    (void)p;
    (void)we;
    if (lId == kIdCancelBtn) {
        InitProgress();
        // TODO: GAMEIN not ported (R-12.x deferred).
    }
}

}  // namespace mxh::ui
