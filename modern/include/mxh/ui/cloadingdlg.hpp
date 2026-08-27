// cloadingdlg.hpp -- modern port of Moxiang CLoadingDlg
// (loading screen dialog: an empty placeholder).
//
// 1:1 port of legacy `CLoadingDlg` from
//   `[Client]MH\LoadingDlg.{h,cpp}`.
//
// Surface (legacy):
//   - Ctor: empty body, no state init.
//   - Dtor: empty body.
//   - NO Linking method (1:1 quirk: the class is the
//     most minimal Tier 2 dialog in the engine -- it
//     exists in the dialog tree to satisfy the
//     "loading" window id during scene transitions).
//   - NO OnActionEvent override.
//   - NO member fields.
//
// Modern port:
//   - Ctor / Dtor: default (1:1 with empty bodies).
//   - No additional members, methods, or behavior.
//
// 1:1 quirks:
//   - 1:1 with legacy CLoadingDlg's empty bodies.
//   - 1:1 with legacy NO Linking: the modern port
//     does not add a Linking method even though
//     other 1:1 ports synthesize one.

#pragma once

#include "cdialog.hpp"

namespace mxh::ui {

class cLoadingDlg : public cDialog {
public:
    cLoadingDlg();
    ~cLoadingDlg() override;

    cLoadingDlg(const cLoadingDlg&) = delete;
    cLoadingDlg& operator=(const cLoadingDlg&) = delete;
};

}  // namespace mxh::ui