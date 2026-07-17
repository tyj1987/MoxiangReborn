// loadingdlg.cpp — 1:1 port of 墨香 CLoadingDlg
// (loading screen placeholder). See loadingdlg.hpp
// for the data-model rationale + 1:1 quirks.

#include "loadingdlg.hpp"

namespace mxh::ui {

cLoadingDlg::cLoadingDlg() {
    // 1:1 with legacy CLoadingDlg::CLoadingDlg:
    //   empty body, no state init.
}

cLoadingDlg::~cLoadingDlg() = default;

}  // namespace mxh::ui
