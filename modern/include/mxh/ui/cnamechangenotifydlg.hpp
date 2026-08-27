// cnamechangenotifydlg.hpp -- modern port of Moxiang
//   CNameChangeNotifyDlg (name change notification
//   placeholder dialog).
//
// 1:1 port of legacy `CNameChangeNotifyDlg` from
//   `[Client]MH\NameChangeNotifyDlg.{h,cpp}`.
//
// Surface (legacy):
//   - Ctor: m_type = WT_NAMECHANGENOTIFY_DLG
//     (legacy cWindow type tag).
//   - Dtor: empty body.
//   - No Linking / OnActionEvent / Render / other
//     methods.
//
// Modern port:
//   - Ctor / Dtor: default (1:1 with empty bodies).
//   - 1:1 quirk: legacy m_type = WT_NAMECHANGENOTIFY_DLG
//     assignment is dropped (modern cWindow does
//     not have m_type field, removed in Phase 6
//     when cWindow was modernized).

#pragma once

#include "cdialog.hpp"

namespace mxh::ui {

class cNameChangeNotifyDlg : public cDialog {
public:
    cNameChangeNotifyDlg();
    ~cNameChangeNotifyDlg() override;

    cNameChangeNotifyDlg(const cNameChangeNotifyDlg&) = delete;
    cNameChangeNotifyDlg& operator=(const cNameChangeNotifyDlg&) = delete;
};

}  // namespace mxh::ui