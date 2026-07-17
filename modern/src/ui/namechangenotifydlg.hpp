// namechangenotifydlg.hpp — modern port of 墨香
// CNameChangeNotifyDlg (name change notification dialog:
// an empty placeholder).
//
// 1:1 port of legacy `CNameChangeNotifyDlg` from
//   `墨香【源码】\[Client]MH\NameChangeNotifyDlg.h` (652 B) and
//   `墨香【源码】\[Client]MH\NameChangeNotifyDlg.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_NAMECHANGENOTIFY_DLG
//     (legacy cWindow type tag).
//   - Dtor: empty body.
//   - No Linking / OnActionEvent / Render / other
//     methods.
//
// The modern port covers:
//   - Ctor / dtor: empty (no-op) — 1:1 with legacy.
//   - 1:1 quirk: legacy m_type = WT_NAMECHANGENOTIFY_DLG
//     assignment is dropped (modern cWindow does
//     not have m_type field, removed in Phase 6
//     when cWindow was modernized).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 23rd **Tier 2** dialog port (after
// cLoadingDlg).

#pragma once

#include "cdialog.hpp"

namespace mxh::ui {

class cNameChangeNotifyDlg : public cDialog {
public:
    cNameChangeNotifyDlg();
    ~cNameChangeNotifyDlg() override;
};

}  // namespace mxh::ui
