// introreplaydlg.cpp — 1:1 port of 墨香 CIntroReplayDlg
// (intro replay dialog: a placeholder with no
// behavior). See introreplaydlg.hpp for the
// data-model rationale + 1:1 quirks.

#include "introreplaydlg.hpp"

namespace mxh::ui {

cIntroReplayDlg::cIntroReplayDlg() {
    // 1:1 with legacy CIntroReplayDlg::CIntroReplayDlg:
    //   empty body, no state init.
}

cIntroReplayDlg::~cIntroReplayDlg() = default;

}  // namespace mxh::ui
