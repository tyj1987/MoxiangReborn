// titanpartsprogressbardlg.hpp — modern port of
// 墨香 CTitanPartsProgressBarDlg (titan-parts make
// progress bar dialog: CProgressBarDlg subclass).
//
// 1:1 port of legacy `CTitanPartsProgressBarDlg` from
//   `墨香【源码】\[Client]MH\TitanPartsProgressBarDlg.h`
//   and `墨香【源码】\[Client]MH\TitanPartsProgressBarDlg.cpp`.
//
// What the legacy does:
//   - Ctor: empty body.
//   - Dtor: empty body.
//   - Linking: resolve CObjectGuagen
//     (m_pProgressGuagen by TITANPARTS_PROGRESSBAR_GAGE)
//     + 1 cStatic (m_pRemaintimeStatic by
//     TITANPARTS_PROGRESSBAR_TIME).
//   - OnActionEvent: switch (lId):
//     case TITANPARTS_PROGRESSBAR_CANCEL →
//     InitProgress() + GAMEIN->GetTitanPartsMakeDlg()
//     ->SetDisable(FALSE).
//
// The modern port covers:
//   - Ctor: empty (no-op).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve 2 children by id,
//     call base setters.
//   - OnActionEvent: TODO (GAMEIN singleton not
//     ported, R-12.x deferred). Modern port calls
//     InitProgress on cancel.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is a Tier 2 dialog port (after
// cTitanMixProgressBarDlg). The dialog has 2
// children (1 CObjectGuagen + 1 cStatic).

#pragma once

#include "progressbardlg.hpp"

#include <cstdint>

namespace mxh::ui {

class cTitanPartsProgressBarDlg : public cProgressBarDlg {
public:
    cTitanPartsProgressBarDlg();
    ~cTitanPartsProgressBarDlg() override;

    // ----- 1:1 with legacy CTitanPartsProgressBarDlg::Linking -----

    // 1:1 with legacy Linking. Resolve 1
    // CObjectGuagen (m_pProgressGuagen by
    // kIdProgressBarGage) + 1 cStatic
    // (m_pRemaintimeStatic by kIdRemaintimeTime).
    void Linking();

    // ----- 1:1 with legacy CTitanPartsProgressBarDlg::OnActionEvent -----

    // 1:1 with legacy OnActionEvent. The cancel
    // branch InitProgress + GAMEIN dispatch is
    // TODO (R-12.x deferred). Modern port calls
    // InitProgress only.
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h
    // TITANPARTS_PROGRESSBAR_GAGE /
    // TITANPARTS_PROGRESSBAR_TIME / cancel id
    // (TITANPARTS_PROGRESSBAR_CANCEL). Local 670-672.
    static constexpr std::int32_t kIdProgressBarGage  = 670;
    static constexpr std::int32_t kIdRemaintimeTime   = 671;
    static constexpr std::int32_t kIdCancelBtn        = 672;
};

}  // namespace mxh::ui
