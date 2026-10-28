// cintroreplaydlg.hpp -- modern port of Moxiang
//   CIntroReplayDlg (intro replay placeholder dialog).
//
// 1:1 port of legacy `CIntroReplayDlg` from
//   `[Client]MH\IntroReplayDlg.{h,cpp}`.
//
// Surface (legacy):
//   - Ctor: empty body.
//   - Dtor: empty body.
//   - Linking: empty body (no children resolved, no
//     state set). The dialog is a 1:1 placeholder
//     -- it exists in the dialog tree to satisfy
//     the window manager's "intro replay button"
//     target id. The actual intro replay logic
//     lives in a separate cinematic manager
//     (R-12.x deferred).
//
// Modern port:
//   - Ctor / Dtor: default (1:1 with empty bodies).
//   - Linking: empty no-op (1:1 with legacy).
//
// 1:1 quirks:
//   - 1:1 with legacy CIntroReplayDlg's empty
//     Linking body (1:1 quirk: the legacy has the
//     method but does nothing in it; the modern
//     port keeps the method for symmetry with the
//     other 1:1 dialogs that use Linking for child
//     resolution).

#pragma once

#include "cdialog.hpp"

namespace mxh::ui {

class cIntroReplayDlg : public cDialog {
public:
    cIntroReplayDlg();
    ~cIntroReplayDlg() override;

    cIntroReplayDlg(const cIntroReplayDlg&) = delete;
    cIntroReplayDlg& operator=(const cIntroReplayDlg&) = delete;

    // ----- 1:1 with legacy CIntroReplayDlg::Linking -----

    // 1:1 with legacy: empty no-op. The dialog is
    // a 1:1 placeholder; the actual intro replay
    // logic is in a separate cinematic manager
    // (R-12.x deferred).
    void Linking() noexcept {}
};

}  // namespace mxh::ui