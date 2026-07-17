// introreplaydlg.hpp — modern port of 墨香 CIntroReplayDlg
// (intro replay dialog: a placeholder dialog with no
// behavior).
//
// 1:1 port of legacy `CIntroReplayDlg` from
//   `墨香【源码】\[Client]MH\IntroReplayDlg.h` (475 B) and
//   `墨香【源码】\[Client]MH\IntroReplayDlg.cpp`.
//
// What the legacy does:
//   - Ctor / dtor: empty bodies (no state init).
//   - Linking: empty body (no children resolved, no
//     state set). The dialog is a 1:1 placeholder —
//     it exists in the dialog tree to satisfy the
//     window manager's "intro replay button" target
//     id (some game flow shows a replay button that
//     re-plays the intro cinematic; the dialog
//     itself does nothing).
//
// The modern port covers:
//   - Ctor / dtor: empty (no-op) — 1:1 with legacy.
//   - Linking: empty no-op (1:1 with legacy).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 20th **Tier 2** dialog port (after
// cChinaAdviceDlg). The dialog has no service
// dependency on the modern service interface
// (Phase 13) — and no singleton dependencies at all
// (Linking is a complete no-op).

#pragma once

#include "cdialog.hpp"

namespace mxh::ui {

class cIntroReplayDlg : public cDialog {
public:
    cIntroReplayDlg();
    ~cIntroReplayDlg() override;

    // ----- 1:1 with legacy CIntroReplayDlg::Linking -----

    // 1:1 with legacy: empty body. The dialog is a
    // 1:1 placeholder — it exists in the dialog
    // tree to satisfy the window manager's "intro
    // replay button" target id but does no work
    // itself.
    void Linking() noexcept {}
};

}  // namespace mxh::ui
