// titanmixprogressbardlg.hpp — modern port of
// 墨香 CTitanMixProgressBarDlg (titan-mix progress
// bar dialog: CProgressBarDlg subclass).
//
// 1:1 port of legacy `CTitanMixProgressBarDlg` from
//   `墨香【源码】\[Client]MH\TitanMixProgressBarDlg.h`
//   and `墨香【源码】\[Client]MH\TitanMixProgressBarDlg.cpp`.
//
// What the legacy does:
//   - Ctor: empty body.
//   - Dtor: empty body.
//   - Linking: resolve CObjectGuagen
//     (m_pProgressGuagen by TITANMIX_PROGRESSBAR_GAGE)
//     + 1 cStatic (m_pRemaintimeStatic by
//     TITANMIX_PROGRESSBAR_TIME).
//   - OnActionEvent: switch (lId):
//     case TITANMIX_PROGRESSBAR_CANCEL →
//     InitProgress() + GAMEIN->GetTitanMixDlg()
//     ->SetDisable(FALSE).
//   - SuccessProcess: empty body (placeholder).
//
// The modern port covers:
//   - Ctor: empty (no-op).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve 2 children by id,
//     call base cProgressBarDlg::SetProgressGuagen +
//     cProgressBarDlg::SetRemaintimeStatic.
//   - OnActionEvent: REAL -- InitProgress() +
//     optional host-injected re-enable callback
//     (replaces legacy GAMEIN->GetTitanMixDlg()
//     ->SetDisable(FALSE)) when SetCancelCallback
//     binds one. With no callback registered the
//     GAMEIN side is safely no-oped.
//   - SuccessProcess: empty (1:1 with legacy
//     placeholder).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is a Tier 2 dialog port (after
// cProgressBarDlg). The dialog has 2 children
// (1 CObjectGuagen + 1 cStatic). The parent
// re-enable side effect is wired through an
// OPTIONAL host callback rather than the legacy
// GAMEIN singleton, so each dialog stays a pure
// UI object decoupled from framework globals.

#pragma once

#include "progressbardlg.hpp"

#include <cstdint>

namespace mxh::ui {

class cTitanMixProgressBarDlg : public cProgressBarDlg {
public:
    cTitanMixProgressBarDlg();
    ~cTitanMixProgressBarDlg() override;

    // ----- 1:1 with legacy CTitanMixProgressBarDlg::Linking -----

    // 1:1 with legacy Linking. Resolve 1
    // CObjectGuagen (m_pProgressGuagen by
    // kIdProgressBarGage) + 1 cStatic
    // (m_pRemaintimeStatic by kIdRemaintimeTime).
    void Linking();

    // ----- 1:1 with legacy CTitanMixProgressBarDlg::OnActionEvent -----

    // 1:1 with legacy OnActionEvent. The cancel
    // branch matches legacy 1:1: InitProgress() is
    // real, then the host-injected re-enable
    // callback (replacing legacy GAMEIN->GetTitanMixDlg()
    // ->SetDisable(FALSE)) is invoked when present.
    // When no callback is registered the cancel
    // branch safely no-ops the GAMEIN side effect.
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    using ReEnableFn = void (*)(void* userData);

    // Replaces legacy GAMEIN->GetTitanMixDlg()->SetDisable(FALSE)
    // invoked from the cancel branch. Optional: when the
    // callback is null the cancel branch is still safe.
    void SetCancelCallback(ReEnableFn reEnable,
                           void* userData = nullptr) noexcept;

    // ----- 1:1 with legacy CTitanMixProgressBarDlg::SuccessProcess -----

    // 1:1 with legacy SuccessProcess. Empty
    // placeholder (1:1 with legacy body).
    void SuccessProcess() {}

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h
    // TITANMIX_PROGRESSBAR_GAGE /
    // TITANMIX_PROGRESSBAR_TIME / cancel id
    // (TITANMIX_PROGRESSBAR_CANCEL). Local 660-662.
    static constexpr std::int32_t kIdProgressBarGage  = 660;
    static constexpr std::int32_t kIdRemaintimeTime   = 661;
    static constexpr std::int32_t kIdCancelBtn        = 662;

private:
    ReEnableFn m_reEnableFn = nullptr;
    void* m_reEnableUserData = nullptr;
};

}  // namespace mxh::ui
