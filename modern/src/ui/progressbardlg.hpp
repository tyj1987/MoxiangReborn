// progressbardlg.hpp — modern port of 墨香
// CProgressBarDlg (base progress bar dialog:
// CObjectGuagen + 1 cStatic remaintime, with
// process/success state).
//
// 1:1 port of legacy `CProgressBarDlg` from
//   `墨香【源码】\[Client]MH\ProgressBarDlg.h` (1072 B)
//   and `墨香【源码】\[Client]MH\ProgressBarDlg.cpp`.
//
// What the legacy does:
//   - Ctor: m_pProgressGuagen=NULL;
//     m_pRemaintimeStatic=NULL; m_bProgressStart=FALSE;
//     m_bSuccessProgress=FALSE; m_dwProcessTime=0;
//     m_dwCurrentTime=0; m_dwSuccessTime=0.
//   - Dtor: m_pProgressGuagen=NULL;
//     m_pRemaintimeStatic=NULL.
//   - SetActive(BOOL val) override: cDialog::SetActive
//     + if val==FALSE: m_bProgressStart=FALSE;
//     m_dwProcessTime=0; m_dwCurrentTime=0;
//     m_pProgressGuagen->SetValue(0, 0).
//   - Process: if !m_bProgressStart return;
//     m_dwCurrentTime = gCurTime; fGageValue = 1.0f
//     - ((m_dwProcessTime - m_dwCurrentTime) / m_dwSuccessTime);
//     m_pProgressGuagen->SetValue(fGageValue, m_dwCurrentTime);
//     if (m_dwProcessTime < m_dwCurrentTime) m_bSuccessProgress=TRUE;
//     sprintf(CHATMGR->GetChatMsg(1043), (m_dwProcessTime
//     - m_dwCurrentTime+1000)/1000) →
//     m_pRemaintimeStatic->SetStaticText.
//   - StartProgress: InitProgress() + m_bProgressStart=TRUE
//     + m_dwCurrentTime=gCurTime; m_dwProcessTime
//     =m_dwCurrentTime+m_dwSuccessTime; SetActive(TRUE).
//   - InitProgress: m_bProgressStart=FALSE;
//     m_bSuccessProgress=FALSE; m_dwProcessTime=0;
//     m_dwCurrentTime=0; SetActive(FALSE).
//   - GetSuccessProgress/SetSuccessProgress: getter/setter
//     for m_bSuccessProgress.
//   - SetSuccessTime(DWORD dwTime): m_dwSuccessTime=dwTime.
//   - Render: Process() + cDialog::Render().
//
// The modern port covers:
//   - Ctor: empty (1:1 quirks: m_pProgressGuagen /
//     m_pRemaintimeStatic null-init via default
//     member init; m_bProgressStart / m_bSuccessProgress
//     init to false; m_dw*State init to 0).
//   - Dtor: empty (no-op).
//   - SetActive override: REAL (idempotent + reset
//     state on val==FALSE; SetValue(0, 0) is REAL
//     on CObjectGuagen).
//   - Process: TODO (gCurTime not ported, R-12.x
//     deferred). Modern port returns without
//     updating m_dwCurrentTime / m_pProgressGuagen
//     / m_pRemaintimeStatic.
//   - StartProgress: TODO (gCurTime not ported).
//     Modern port calls InitProgress() + sets
//     m_bProgressStart=true + SetActive(true) (without
//     the gCurTime-based m_dwCurrentTime/m_dwProcessTime
//     updates).
//   - InitProgress: REAL (clears state + SetActive(false)).
//   - GetSuccessProgress/SetSuccessProgress: REAL.
//   - SetSuccessTime: REAL.
//   - Render: TODO (Process is TODO + cDialog::Render
//     is no-op). Modern port returns without
//     calling Process.
//   - Note: m_pProgressGuagen is set by subclass
//     Linking (each subclass does its own Linking);
//     the base CProgressBarDlg does NOT have its
//     own Linking. The data fields are stored
//     as raw pointers (non-owning, 1:1 with legacy).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the base class for 3 progress bar dialogs
// (TitanPartsProgressBarDlg / TitanMixProgressBarDlg
// / UniqueItemMixProgressBarDlg). The dialog has
// CObjectGuagen (m_pProgressGuagen) + 1 cStatic
// (m_pRemaintimeStatic) + 6 state fields. gCurTime +
// CHATMGR are R-12.x deferred.

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cStatic;
class cObjectGuagen;

class cProgressBarDlg : public cDialog {
public:
    cProgressBarDlg();
    ~cProgressBarDlg() override;

    // ----- 1:1 with legacy CProgressBarDlg::SetActive override -----

    // 1:1 with legacy SetActive override. If val ==
    // FALSE, reset m_bProgressStart / m_dwProcessTime
    // / m_dwCurrentTime + m_pProgressGuagen->SetValue
    // (0, 0). Always calls base cDialog::SetActive.
    void SetActive(bool val) noexcept override;

    // ----- 1:1 with legacy CProgressBarDlg::Process -----

    // 1:1 with legacy Process. The gCurTime-based
    // tick + SetValue + SetStaticText (CHATMGR msg
    // 1043) is TODO (R-12.x deferred). Modern port
    // returns without updating state.
    void Process();

    // ----- 1:1 with legacy CProgressBarDlg::StartProgress -----

    // 1:1 with legacy StartProgress. Calls
    // InitProgress + sets m_bProgressStart = true +
    // SetActive(true). The gCurTime-based
    // m_dwCurrentTime / m_dwProcessTime updates are
    // TODO (R-12.x deferred).
    void StartProgress();

    // ----- 1:1 with legacy CProgressBarDlg::InitProgress -----

    // 1:1 with legacy InitProgress. Resets 4 state
    // fields + SetActive(false).
    void InitProgress();

    // ----- 1:1 with legacy CProgressBarDlg::GetSuccessProgress -----

    // 1:1 with legacy GetSuccessProgress. Returns
    // m_bSuccessProgress.
    bool GetSuccessProgress() const noexcept {
        return m_bSuccessProgress;
    }

    // ----- 1:1 with legacy CProgressBarDlg::SetSuccessProgress -----

    // 1:1 with legacy SetSuccessProgress. Sets
    // m_bSuccessProgress.
    void SetSuccessProgress(bool bVal) noexcept {
        m_bSuccessProgress = bVal;
    }

    // ----- 1:1 with legacy CProgressBarDlg::SetSuccessTime -----

    // 1:1 with legacy SetSuccessTime(DWORD dwTime).
    // Sets m_dwSuccessTime.
    void SetSuccessTime(std::uint32_t dwTime) noexcept {
        m_dwSuccessTime = dwTime;
    }

    // ----- 1:1 with legacy CProgressBarDlg::Render -----

    // 1:1 with legacy Render. The Process() call is
    // TODO (R-12.x deferred); cDialog::Render is
    // no-op. Modern port is a no-op.
    void Render() override;

    // ----- 1:1 with legacy state accessors (subclass Linking) -----

    // Subclass Linking should set m_pProgressGuagen
    // and m_pRemaintimeStatic. The base class doesn't
    // have its own Linking (each subclass does).
    cObjectGuagen* GetProgressGuagen() const noexcept {
        return m_pProgressGuagen;
    }
    cStatic* GetRemaintimeStatic() const noexcept {
        return m_pRemaintimeStatic;
    }

    // Subclass Linking setters (non-owning raw pointer,
    // dialog owns the child via its cWindow children
    // list).
    void SetProgressGuagen(cObjectGuagen* g) noexcept {
        m_pProgressGuagen = g;
    }
    void SetRemaintimeStatic(cStatic* s) noexcept {
        m_pRemaintimeStatic = s;
    }

    // State accessors (used by tests + subclasses).
    bool IsProgressStart() const noexcept {
        return m_bProgressStart;
    }
    std::uint32_t GetProcessTime() const noexcept {
        return m_dwProcessTime;
    }
    std::uint32_t GetCurrentTime() const noexcept {
        return m_dwCurrentTime;
    }
    std::uint32_t GetSuccessTime() const noexcept {
        return m_dwSuccessTime;
    }

private:
    // 1:1 with legacy m_pProgressGuagen. The base
    // class doesn't have its own Linking; subclass
    // Linking sets this field. The dialog does NOT
    // own this pointer (it's a child of the dialog
    // tree).
    cObjectGuagen* m_pProgressGuagen = nullptr;

    // 1:1 with legacy m_pRemaintimeStatic (resolved
    // by subclass Linking).
    cStatic* m_pRemaintimeStatic = nullptr;

    // 1:1 with legacy m_bProgressStart (BOOL, init
    // FALSE).
    bool m_bProgressStart = false;

    // 1:1 with legacy m_bSuccessProgress (BOOL, init
    // FALSE).
    bool m_bSuccessProgress = false;

    // 1:1 with legacy m_dwProcessTime (DWORD, init 0).
    std::uint32_t m_dwProcessTime = 0;

    // 1:1 with legacy m_dwCurrentTime (DWORD, init 0).
    std::uint32_t m_dwCurrentTime = 0;

    // 1:1 with legacy m_dwSuccessTime (DWORD, init 0).
    std::uint32_t m_dwSuccessTime = 0;
};

}  // namespace mxh::ui
