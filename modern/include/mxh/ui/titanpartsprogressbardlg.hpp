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
//   - OnActionEvent: REAL -- InitProgress() +
//     optional host-injected re-enable callback
//     (replaces legacy GAMEIN->GetTitanPartsMakeDlg()
//     ->SetDisable(FALSE)) when SetCancelCallback
//     binds one. With no callback registered the
//     GAMEIN side is safely no-oped.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is a Tier 2 dialog port (after
// cTitanMixProgressBarDlg). The dialog has 2
// children (1 CObjectGuagen + 1 cStatic). The
// parent re-enable side effect is wired through
// an OPTIONAL host callback rather than the legacy
// GAMEIN singleton.

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
    // branch matches legacy 1:1: InitProgress() is
    // real, then the host-injected re-enable
    // callback (replacing legacy GAMEIN->GetTitanPartsMakeDlg()
    // ->SetDisable(FALSE)) is invoked when present.
    // When no callback is registered the cancel
    // branch safely no-ops the GAMEIN side effect.
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    using ReEnableFn = void (*)(void* userData);

    // Replaces legacy GAMEIN->GetTitanPartsMakeDlg()->SetDisable(FALSE)
    // invoked from the cancel branch. Optional: when the
    // callback is null the cancel branch is still safe.
    void SetCancelCallback(ReEnableFn reEnable,
                           void* userData = nullptr) noexcept;

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h
    // TITANPARTS_PROGRESSBAR_GAGE /
    // TITANPARTS_PROGRESSBAR_TIME / cancel id
    // (TITANPARTS_PROGRESSBAR_CANCEL). Local 670-672.
    static constexpr std::int32_t kIdProgressBarGage  = 670;
    static constexpr std::int32_t kIdRemaintimeTime   = 671;
    static constexpr std::int32_t kIdCancelBtn        = 672;

private:
    ReEnableFn m_reEnableFn = nullptr;
    void* m_reEnableUserData = nullptr;
};

}  // namespace mxh::ui
