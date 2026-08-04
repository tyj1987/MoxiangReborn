// shoutchatdialog.hpp — modern port of 墨香
// CShoutchatDialog (shout chat log dialog: 1 cListDialog
// + m_LastMsgTime state).
//
// 1:1 port of legacy `CShoutchatDialog` from
//   `墨香【源码】\[Client]MH\ShoutchatDialog.h` and
//   `墨香【源码】\[Client]MH\ShoutchatDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_SHOUTCHAT_DLG; m_LastMsgTime = 0.
//   - Dtor: empty body.
//   - Linking: resolve 1 cListDialog
//     (m_pMsgListDlg by CHA_LIST); if
//     GAMERESRCMNGR->IsLowResolution() then
//     GAMEIN->GetShoutchatDlg()->RefreshPosition().
//   - Process: if gCurTime - m_LastMsgTime < 5000
//     return; m_LastMsgTime = gCurTime.
//   - SetActive(BOOL val) override: if val
//     RefreshPosition(); cDialog::SetActive(val).
//   - AddMsg(char* pstr): strncpy to 60-char buf +
//     m_pMsgListDlg->AddItem(buf, RGBA_MAKE(217, 206, 247, 255)).
//   - RefreshPosition: GAMEIN->GetChatDialog()
//     ->GetAbsX() / GetSheetPosY() + SetAbsXY.
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_type drop, m_LastMsgTime
//     init via default member init).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve cListDialog by id.
//     The GAMERESRCMNGR + GAMEIN dispatch is TODO.
//   - Process: 1:1 with legacy 5 sec throttle
//     (gCurTime - m_LastMsgTime < kMsgThrottleMs
//     early return; else update m_LastMsgTime)
//     via OPTIONAL host clock provider.
//   - SetActive override: TODO (RefreshPosition is
//     TODO). Modern port always base SetActive.
//   - AddMsg: REAL — strncpy 60-char buf + AddItem
//     with kShoutchatItemColor (0xFFD9CEF7).
//   - RefreshPosition: TODO (GAMEIN + cChatDialog
//     singletons, R-12.x deferred). Modern port is
//     no-op.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 55th **Tier 2** dialog port (after
// cDebugDlg). The dialog has 1 cListDialog +
// m_LastMsgTime state. GAMERESRCMNGR + GAMEIN +
// cChatDialog + gCurTime are R-12.x deferred.

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cListDialog;

// Shared clock provider signature (replaces legacy gCurTime global).
using ScClockFn = std::uint32_t (*)(void* userData);

class cShoutchatDialog : public cDialog {
public:
    cShoutchatDialog();
    ~cShoutchatDialog() override;

    // ----- 1:1 with legacy CShoutchatDialog::Linking -----

    // 1:1 with legacy Linking. Resolve 1 cListDialog
    // (m_pMsgListDlg by kIdMsgList). The
    // GAMERESRCMNGR + GAMEIN dispatch is TODO.
    void Linking();

    // ----- 1:1 with legacy CShoutchatDialog::Process -----

    // 1:1 with legacy Process. The gCurTime-based
    // 5 sec throttle is REAL via OPTIONAL host clock
    // provider. A null provider preserves the safe
    // no-throttle fallback.
    void Process();

    // Replace the legacy gCurTime read for Process.
    void SetCurrentTimeProvider(ScClockFn getCurrentTime,
                                void* userData = nullptr) noexcept;

    // ----- 1:1 with legacy CShoutchatDialog::SetActive override -----

    // 1:1 with legacy SetActive override. The
    // RefreshPosition is TODO (R-12.x deferred).
    // Modern port always base SetActive.
    void SetActive(bool val) noexcept override;

    // ----- 1:1 with legacy CShoutchatDialog::AddMsg -----

    // 1:1 with legacy AddMsg(char* pstr). strncpy
    // to 60-char buf + AddItem with kShoutchatItemColor.
    void AddMsg(const char* pstr);

    // ----- 1:1 with legacy CShoutchatDialog::RefreshPosition -----

    // 1:1 with legacy RefreshPosition. The
    // GAMEIN + cChatDialog singletons are TODO
    // (R-12.x deferred). Modern port is no-op.
    void RefreshPosition() {}

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h CHA_LIST.
    // Local 730 — distinct from 200-720 used by
    // previous Tier 2 dialogs.
    static constexpr std::int32_t kIdMsgList = 730;

    // 1:1 with legacy RGBA_MAKE(217, 206, 247, 255)
    // for the shout chat item color. ARGB =
    // 0xFFD9CEF7.
    static constexpr std::uint32_t kShoutchatItemColor = 0xFFD9CEF7u;

    // 1:1 with legacy Process 5 sec timer.
    static constexpr std::uint32_t kMsgThrottleMs = 5000;

    // 1:1 with legacy strncpy 60-char buf (60+1 for null).
    static constexpr std::size_t kMaxMsgLen = 60;

    // 1:1 with legacy m_LastMsgTime getter (test-only).
    std::uint32_t GetLastMsgTime() const noexcept { return m_LastMsgTime; }

private:
    // 1:1 with legacy m_pMsgListDlg (resolved in
    // Linking by CHA_LIST id).
    cListDialog* m_pMsgListDlg = nullptr;

    // 1:1 with legacy m_LastMsgTime (DWORD; init 0
    // in ctor). Modern port uses std::uint32_t.
    std::uint32_t m_LastMsgTime = 0;

    ScClockFn m_getCurrentTimeFn = nullptr;
    void*     m_clockUserData    = nullptr;
};

}  // namespace mxh::ui
