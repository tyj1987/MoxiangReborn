// ctitanchangepreviewdlg.hpp -- modern port of Moxiang
//   CTitanChangePreViewDlg (titan parts change preview
//   placeholder dialog).
//
// 1:1 port of legacy CTitanChangePreViewDlg from
//   [Client]MH/TitanChangePreView.{h,cpp}.
//   (Note: legacy filename has no Dlg suffix but
//   the class itself is CTitanChangePreViewDlg.)
//
// Surface (legacy):
//   - Ctor: empty body, no state init.
//   - Dtor: empty body.
//   - Linking: empty body (1:1 quirk: the dialog has
//     no children to resolve).
//   - SetActive override: forwards to
//     cDialog::SetActive(val) (1:1 with legacy).
//   - No OnActionEvent / Render / other methods
//     of its own (1:1 with legacy).
//   - No member fields.
//
// WindowID (legacy WindowIDs.h, line 1122):
//   TITANPARTSCHANGEPREVIEW_DLG = 1122
//
// Modern port:
//   - Ctor / Dtor: default (1:1 with empty bodies).
//   - Linking override: empty body (1:1 with legacy).
//   - SetActive override: forwards to base (1:1).
//   - No additional members, methods, or behavior.
//   - No m_type assignment: modern cWindow does
//     not have m_type (removed in Phase 6).
//
// 1:1 quirks:
//   - 1:1 with legacy empty ctor/dtor bodies.
//   - 1:1 with legacy empty Linking.
//   - 1:1 with legacy SetActive that just forwards
//     to the base cDialog::SetActive.

#pragma once

#include "cdialog.hpp"

namespace mxh::ui {

class cTitanChangePreViewDlg : public cDialog {
public:
    cTitanChangePreViewDlg();
    ~cTitanChangePreViewDlg() override;

    cTitanChangePreViewDlg(const cTitanChangePreViewDlg&) = delete;
    cTitanChangePreViewDlg& operator=(const cTitanChangePreViewDlg&) = delete;

    // 1:1 with legacy CTitanChangePreViewDlg::Linking.
    // Empty body: the dialog has no children to
    // resolve.
    void Linking();

    // 1:1 with legacy CTitanChangePreViewDlg::SetActive
    // override. Forwards to cDialog::SetActive(val).
    void SetActive(bool val) noexcept override;

    // 1:1 with legacy WindowIDs.h
    // TITANPARTSCHANGEPREVIEW_DLG.
    static constexpr std::int32_t kWindowId = 1122;
};

}  // namespace mxh::ui
