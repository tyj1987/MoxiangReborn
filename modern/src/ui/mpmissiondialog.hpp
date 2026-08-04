// mpmissiondialog.hpp — modern port of 墨香
// CMPMissionDialog (event-map mission notice
// dialog: 2 cTextArea + array of mission/caution
// messages).
//
// 1:1 port of legacy `CMPMissionDialog` from
//   `墨香【源码】\[Client]MH\MPMissionDialog.h` (993 B)
//   and `墨香【源码】\[Client]MH\MPMissionDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_MPMISSIONDLG;
//     ZeroMemory(m_pMissionMsg[5]);
//     ZeroMemory(m_pCautionMsg[5]);
//     LoadMissionMsg is COMMENTED OUT in ctor
//     (was a 1:1 quirk — the legacy defers
//     LoadMissionMsg).
//   - Dtor: empty body.
//   - Linking: resolve 2 cTextArea
//     (m_pMission by MP_MMISSION, m_pCaution by
//     MP_MCAUTION); SetScriptText on each with
//     CHATMGR->GetChatMsg(665/666); init
//     m_dwStartTime = 0.
//   - SetMissionInfo(int msgnum): ASSERT 0 if
//     msgnum >= MAX_MISSIONMSG_NUM (5), else
//     m_pMission->SetScriptText(m_pMissionMsg[msgnum]);
//     m_pCaution->SetScriptText(m_pCautionMsg[msgnum]).
//   - SetActive override: if val == FALSE
//     → GAMEIN->GetMPNoticeDialog()->SetActive(TRUE);
//     else m_dwStartTime = gCurTime;
//     then cDialog::SetActive(val).
//   - LoadMissionMsg: CMHFile::Init("./Resource/SuryunMissionMsg.bin")
//     [COMMENTED OUT in legacy cpp body — the
//     legacy doesn't actually implement it; the
//     m_pMissionMsg/m_pCautionMsg arrays are
//     never populated].
//   - ActionEvent: cDialog::ActionEvent; if
//     IsActive and gCurTime-m_dwStartTime >= 5000
//     → SetActive(FALSE).
//
// The modern port covers:
//   - Ctor: empty (1:1 quirks: m_type drop,
//     m_pMissionMsg/m_pCautionMsg arrays are
//     commented-out / never populated in legacy;
//     modern port uses std::vector<std::string>
//     of size 0; LoadMissionMsg is a no-op since
//     the file format is not ported).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve 2 cTextArea by id,
//     SetScriptText with placeholders for
//     CHATMGR->GetChatMsg(665/666).
//   - SetMissionInfo: REAL (defensive bounds-check
//     returns without action if msgnum OOB).
//   - SetActive override: 1:1 with legacy val==TRUE
//     gCurTime stamp via OPTIONAL host clock
//     provider; the val==FALSE
//     GAMEIN->GetMPNoticeDialog dispatch is TODO.
//   - ActionEvent: 1:1 with legacy 5 sec gate
//     (auto-close when
//     gCurTime - m_dwStartTime >= kAutoCloseMs);
//     the base cDialog::ActionEvent dispatch is
//     TODO (CMouse not ported, R-12.x deferred).
//   - 1:1 quirk: legacy cpp's LoadMissionMsg body
//     is empty (just opens a file and returns). The
//     m_pMissionMsg/m_pCautionMsg arrays stay
//     NULL throughout. Modern port uses
//     std::vector<std::string>(0) — same
//     effective state.

#pragma once

#include "cdialog.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mxh::ui {

class cTextArea;

// Shared clock provider signature (replaces legacy `gCurTime` global).
using MpClockFn = std::uint32_t (*)(void* userData);

class cMPMissionDialog : public cDialog {
public:
    cMPMissionDialog();
    ~cMPMissionDialog() override;

    // ----- 1:1 with legacy CMPMissionDialog::Linking -----

    // 1:1 with legacy Linking. Resolve 2 cTextArea
    // (m_pMission by kIdMission, m_pCaution by
    // kIdCaution), call SetScriptText on each with
    // the placeholder strings (CHATMGR->GetChatMsg
    // (665/666) substitution).
    void Linking();

    // ----- 1:1 with legacy CMPMissionDialog::SetMissionInfo -----

    // 1:1 with legacy SetMissionInfo(int msgnum).
    // The legacy asserts if msgnum >=
    // MAX_MISSIONMSG_NUM (5). Modern port uses a
    // defensive bounds-check (no assert — returns
    // without action). Sets m_pMission->SetScriptText
    // (m_pMissionMsg[msgnum]) + m_pCaution->SetScriptText
    // (m_pCautionMsg[msgnum]).
    void SetMissionInfo(int msgnum);

    // 1:1 with legacy m_dwStartTime getter (test-only).
    std::uint32_t GetStartTime() const noexcept { return m_dwStartTime; }

    // ----- 1:1 with legacy CMPMissionDialog::SetActive override -----

    // 1:1 with legacy SetActive override.
    // val==TRUE: stamp m_dwStartTime via OPTIONAL
    // host clock provider (legacy gCurTime).
    // val==FALSE: GAMEIN->GetMPNoticeDialog dispatch
    // is TODO (R-12.x deferred).
    void SetActive(bool val) noexcept override;

    // Replace the legacy gCurTime read for SetActive +
    // ActionEvent. A null provider preserves the
    // safe zero-clock fallback.
    void SetCurrentTimeProvider(MpClockFn getCurrentTime,
                                void* userData = nullptr) noexcept;

    // ----- 1:1 with legacy CMPMissionDialog::ActionEvent -----

    // 1:1 with legacy ActionEvent. The gCurTime-
    // based 5 sec auto-close gate is REAL (via
    // OPTIONAL host clock provider). The CMouse-
    // based cDialog::ActionEvent dispatch is
    // TODO (CMouse not ported, R-12.x deferred).
    std::uint32_t ActionEvent();

    // ----- 1:1 with legacy CMPMissionDialog::LoadMissionMsg -----

    // 1:1 with legacy LoadMissionMsg. The legacy
    // opens a file but never populates the arrays;
    // modern port is a no-op.
    void LoadMissionMsg() noexcept {}

    // ----- Constants -----

    // 1:1 with legacy MAX_MISSIONMSG_NUM = 5.
    static constexpr int kMaxMissionMsgNum = 5;

    // 1:1 with legacy WindowIDs.h MP_MMISSION /
    // MP_MCAUTION (570-571). Local 570-571 —
    // distinct from 200-560 used by previous Tier 2
    // dialogs.
    static constexpr std::int32_t kIdMission = 570;
    static constexpr std::int32_t kIdCaution = 571;

    // 1:1 with legacy CHATMGR->GetChatMsg(665) for
    // mission text + CHATMGR->GetChatMsg(666) for
    // caution text. Modern port uses literal
    // placeholders until CHATMGR is ported.
    static constexpr const char* kMissionText =
        "MP_MISSION_TEXT";  // CHATMGR msg 665
    static constexpr const char* kCautionText =
        "MP_CAUTION_TEXT";  // CHATMGR msg 666

private:
    // 1:1 with legacy m_pMission (resolved in
    // Linking by MP_MMISSION id).
    cTextArea* m_pMission = nullptr;

    // 1:1 with legacy m_pCaution (resolved in
    // Linking by MP_MCAUTION id).
    cTextArea* m_pCaution = nullptr;

    // 1:1 with legacy m_pMissionMsg[MAX_MISSIONMSG_NUM=5].
    // Legacy: char* (NULL after ctor ZeroMemory).
    // Modern: std::vector<std::string> (size 0 after
    // ctor; LoadMissionMsg is a no-op).
    std::vector<std::string> m_pMissionMsg;

    // 1:1 with legacy m_pCautionMsg[MAX_MISSIONMSG_NUM=5].
    // Legacy: char* (NULL after ctor ZeroMemory).
    // Modern: std::vector<std::string> (size 0 after
    // ctor; LoadMissionMsg is a no-op).
    std::vector<std::string> m_pCautionMsg;

    // 1:1 with legacy m_dwStartTime (DWORD; init 0
    // in Linking). Modern port uses std::uint32_t.
    std::uint32_t m_dwStartTime = 0;

    MpClockFn m_getCurrentTimeFn = nullptr;
    void*     m_clockUserData    = nullptr;
};

}  // namespace mxh::ui
