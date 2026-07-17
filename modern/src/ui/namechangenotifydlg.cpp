// namechangenotifydlg.cpp — 1:1 port of 墨香
// CNameChangeNotifyDlg (name change notification
// placeholder). See namechangenotifydlg.hpp for
// the data-model rationale + 1:1 quirks.

#include "namechangenotifydlg.hpp"

namespace mxh::ui {

cNameChangeNotifyDlg::cNameChangeNotifyDlg() {
    // 1:1 with legacy CNameChangeNotifyDlg::CNameChangeNotifyDlg:
    //   m_type = WT_NAMECHANGENOTIFY_DLG;
    //
    // 1:1 quirk: modern cWindow does not have
    // m_type field (removed in Phase 6 when cWindow
    // was modernized). The ctor body is dropped.
}

cNameChangeNotifyDlg::~cNameChangeNotifyDlg() = default;

}  // namespace mxh::ui
