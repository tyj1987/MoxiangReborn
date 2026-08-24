// uniqueitemmixprogressbardlg.hpp — modern port of
// 墨香 CUniqueItemMixProgressBarDlg (unique-item
// mix progress bar dialog: CProgressBarDlg subclass).
//
// 1:1 port of legacy `CUniqueItemMixProgressBarDlg` from
//   `墨香【源码】\[Client]MH\UniqueItemMixProgressBarDlg.h`
//   and `墨香【源码】\[Client]MH\UniqueItemMixProgressBarDlg.cpp`.
//
// What the legacy does:
//   - Ctor: empty body.
//   - Dtor: empty body.
//   - Linking: resolve CObjectGuagen
//     (m_pProgressGuagen by UNIQUEITEMMIX_PROGRESS_GAGE)
//     + 1 cStatic (m_pRemaintimeStatic by
//     UNIQUEITEMMIX_PROGRESSBAR_TIME).
//   - OnActionEvent: switch (lId):
//     case UNIQUEITEMMIX_PROGRESSBAR_CANCEL →
//     InitProgress() + GAMEIN->GetUniqueItemMixDlg()
//     ->SetDisable(FALSE).
//
// The modern port covers:
//   - Ctor: empty (no-op).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve 2 children by id.
//   - OnActionEvent: REAL -- InitProgress() +
//     optional host-injected re-enable callback
//     (replaces legacy GAMEIN->GetUniqueItemMixDlg()
//     ->SetDisable(FALSE)) when SetCancelCallback
//     binds one. With no callback registered the
//     GAMEIN side is safely no-oped.

#pragma once

#include "progressbardlg.hpp"

#include <cstdint>

namespace mxh::ui {

class cUniqueItemMixProgressBarDlg : public cProgressBarDlg {
public:
    cUniqueItemMixProgressBarDlg();
    ~cUniqueItemMixProgressBarDlg() override;

    // ----- 1:1 with legacy CUniqueItemMixProgressBarDlg::Linking -----

    // 1:1 with legacy Linking. Resolve 1
    // CObjectGuagen (m_pProgressGuagen by
    // kIdProgressBarGage) + 1 cStatic
    // (m_pRemaintimeStatic by kIdRemaintimeTime).
    void Linking();

    // ----- 1:1 with legacy CUniqueItemMixProgressBarDlg::OnActionEvent -----

    // 1:1 with legacy OnActionEvent. The cancel
    // branch matches legacy 1:1: InitProgress() is
    // real, then the host-injected re-enable
    // callback (replacing legacy GAMEIN->GetUniqueItemMixDlg()
    // ->SetDisable(FALSE)) is invoked when present.
    // When no callback is registered the cancel
    // branch safely no-ops the GAMEIN side effect.
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    using ReEnableFn = void (*)(void* userData);

    // Replaces legacy GAMEIN->GetUniqueItemMixDlg()->SetDisable(FALSE)
    // invoked from the cancel branch. Optional: when the
    // callback is null the cancel branch is still safe.
    void SetCancelCallback(ReEnableFn reEnable,
                           void* userData = nullptr) noexcept;

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h
    // UNIQUEITEMMIX_PROGRESS_GAGE /
    // UNIQUEITEMMIX_PROGRESSBAR_TIME / cancel id
    // (UNIQUEITEMMIX_PROGRESSBAR_CANCEL). Local 680-682.
    static constexpr std::int32_t kIdProgressBarGage  = 680;
    static constexpr std::int32_t kIdRemaintimeTime   = 681;
    static constexpr std::int32_t kIdCancelBtn        = 682;

private:
    ReEnableFn m_reEnableFn = nullptr;
    void* m_reEnableUserData = nullptr;
};

}  // namespace mxh::ui
