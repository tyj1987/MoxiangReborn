// loadingdlg.hpp — modern port of 墨香 CLoadingDlg
// (loading screen dialog: an empty placeholder).
//
// 1:1 port of legacy `CLoadingDlg` from
//   `墨香【源码】\[Client]MH\LoadingDlg.h` (578 B) and
//   `墨香【源码】\[Client]MH\LoadingDlg.cpp`.
//
// What the legacy does:
//   - Ctor / dtor: empty bodies (no state init, no
//     child resolution, no Linking override).
//   - The class has NO Linking method (just ctor +
//     dtor). It's a complete 1:1 placeholder
//     existing in the dialog tree to satisfy the
//     "loading" window id during scene transitions.
//
// The modern port covers:
//   - Ctor / dtor: empty (no-op) — 1:1 with legacy.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 22nd **Tier 2** dialog port (after
// cKeySettingTipDlg). The dialog has no service
// dependency on the modern service interface
// (Phase 13) — and no singleton dependencies at all
// (no Linking method).

#pragma once

#include "cdialog.hpp"

namespace mxh::ui {

class cLoadingDlg : public cDialog {
public:
    cLoadingDlg();
    ~cLoadingDlg() override;
};

}  // namespace mxh::ui
