// ctitanchangepreviewdlg.cpp -- modern implementation of
//   Moxiang CTitanChangePreViewDlg (titan parts change
//   preview placeholder dialog).

#include "ctitanchangepreviewdlg.hpp"

namespace mxh::ui {

cTitanChangePreViewDlg::cTitanChangePreViewDlg() = default;

cTitanChangePreViewDlg::~cTitanChangePreViewDlg() = default;

void cTitanChangePreViewDlg::Linking() {
    // 1:1 with legacy empty Linking body: the dialog
    // has no children to resolve.
}

void cTitanChangePreViewDlg::SetActive(bool val) noexcept {
    // 1:1 with legacy SetActive that just forwards
    // to the base cDialog::SetActive(val).
    cDialog::SetActive(val);
}

}  // namespace mxh::ui
