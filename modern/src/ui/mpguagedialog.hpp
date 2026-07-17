// mpguagedialog.hpp — modern port of 墨香
// CMPGuageDialog (event-map timer + experience
// gauge dialog: CObjectGuagen + 3 cStatic).
//
// 1:1 port of legacy `CMPGuageDialog` from
//   `墨香【源码】\[Client]MH\MPGuageDialog.h` (899 B)
//   and `墨香【源码】\[Client]MH\MPGuageDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_MPGUAGEDLG.
//   - Dtor: empty body.
//   - Linking: resolve CObjectGuagen (m_ExpGuage
//     by MP_GEXPGUAGE) + 3 cStatic (m_Time by
//     MP_GTIME, m_ExpPercent by MP_GEXPERCENT,
//     m_pTitle by MP_TITLE).
//   - SetExpGuage(float Percent):
//     m_ExpGuage->SetValue(Percent, 0);
//     sprintf "%4.2f%%" → m_ExpPercent->SetStaticText.
//   - SetTime(DWORD RemainTime): if RemainTime
//     < 30000 → m_Time->SetFGColor(RGB_HALF(255,0,0));
//     sprintf "%02d:%02d" → m_Time->SetStaticText.
//   - SetEventMapTimer(RemainTime, bFlag): switch
//     on bFlag: 0=blue, 1=conditional red, 2=blue;
//     sprintf "%02d:%02d" → m_Time->SetStaticText.
//   - ShowEventMap: SetActive(TRUE) +
//     m_pTitle->SetStaticText(CHATMGR->GetChatMsg(140)).
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_type drop).
//   - Dtor: empty.
//   - Linking: REAL — resolve CObjectGuagen +
//     3 cStatic by id.
//   - SetExpGuage: TODO (CObjectGuagen::SetValue
//     not ported, R-12.x deferred). Modern port
//     only updates m_ExpPercent text.
//   - SetTime: REAL — SetFGColor + SetStaticText
//     on m_Time.
//   - SetEventMapTimer: REAL — 3-way switch on
//     bFlag with SetFGColor + SetStaticText.
//   - ShowEventMap: TODO (CHATMGR not ported,
//     R-12.x deferred). Modern port calls
//     SetActive(true) + SetStaticText with
//     "EVENT_MAP_TITLE" placeholder.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 43rd **Tier 2** dialog port (after
// cAlertDlg). The dialog has CObjectGuagen
// (m_ExpGuage, unported) + 3 cStatic (m_Time +
// m_ExpPercent + m_pTitle). CHATMGR + CObjectGuagen
// are R-12.x deferred.

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cStatic;

class cMPGuageDialog : public cDialog {
public:
    cMPGuageDialog();
    ~cMPGuageDialog() override;

    // ----- 1:1 with legacy CMPGuageDialog::Linking -----

    // 1:1 with legacy Linking. Resolve 1
    // CObjectGuagen (m_ExpGuage by kIdExpGuage) +
    // 3 cStatic (m_Time by kIdTime, m_ExpPercent
    // by kIdExpPercent, m_pTitle by kIdTitle).
    // The CObjectGuagen is forward-declared and
    // stored as void* (1:1 with legacy class-ptr
    // pattern; CObjectGuagen not ported).
    void Linking();

    // ----- 1:1 with legacy CMPGuageDialog::SetExpGuage -----

    // 1:1 with legacy SetExpGuage(float Percent).
    // The CObjectGuagen->SetValue call is TODO
    // (R-12.x deferred). Modern port updates
    // m_ExpPercent text with sprintf "%4.2f%%".
    void SetExpGuage(float percent);

    // ----- 1:1 with legacy CMPGuageDialog::SetTime -----

    // 1:1 with legacy SetTime(DWORD RemainTime).
    // If RemainTime < 30000 → m_Time->SetFGColor
    // (red); sprintf "%02d:%02d" → m_Time->SetStaticText.
    void SetTime(std::uint32_t remainTime);

    // ----- 1:1 with legacy CMPGuageDialog::SetEventMapTimer -----

    // 1:1 with legacy SetEventMapTimer(RemainTime,
    // bFlag). 3-way switch on bFlag: 0=blue,
    // 1=conditional red (if RemainTime < 30000),
    // 2=blue; sprintf "%02d:%02d" → m_Time->SetStaticText.
    void SetEventMapTimer(std::uint32_t remainTime, std::uint8_t bFlag);

    // ----- 1:1 with legacy CMPGuageDialog::ShowEventMap -----

    // 1:1 with legacy ShowEventMap. SetActive(TRUE)
    // + m_pTitle->SetStaticText(kEventMapTitle
    // placeholder for CHATMGR->GetChatMsg(140)).
    void ShowEventMap();

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h MP_GEXPGUAGE /
    // MP_GTIME / MP_GEXPERCENT / MP_TITLE (610-613).
    // Local 610-613 — distinct from 200-610 used
    // by previous Tier 2 dialogs.
    static constexpr std::int32_t kIdExpGuage  = 610;
    static constexpr std::int32_t kIdTime      = 611;
    static constexpr std::int32_t kIdExpPercent = 612;
    static constexpr std::int32_t kIdTitle     = 613;

    // 1:1 with legacy CHATMGR->GetChatMsg(140) for
    // event map title. Modern port uses literal
    // placeholder until CHATMGR is ported.
    static constexpr const char* kEventMapTitle =
        "EVENT_MAP_TITLE";  // CHATMGR msg 140

    // 1:1 with legacy 30000 (DWORD threshold for
    // red text in SetTime / SetEventMapTimer case 1).
    static constexpr std::uint32_t kRedTextThreshold = 30000;

    // 1:1 with legacy bFlag enum (0=ready, 1=active,
    // 2=stopped). Inlined as 3 constants.
    static constexpr std::uint8_t kFlagReady   = 0;
    static constexpr std::uint8_t kFlagActive  = 1;
    static constexpr std::uint8_t kFlagStopped = 2;

private:
    // 1:1 with legacy m_ExpGuage (resolved in
    // Linking by MP_GEXPGUAGE id). CObjectGuagen
    // is forward-declared; modern port stores as
    // void* (untyped pointer, R-12.x deferred).
    void* m_ExpGuage = nullptr;

    // 1:1 with legacy m_Time (resolved in Linking
    // by MP_GTIME id).
    cStatic* m_Time = nullptr;

    // 1:1 with legacy m_ExpPercent (resolved in
    // Linking by MP_GEXPERCENT id).
    cStatic* m_ExpPercent = nullptr;

    // 1:1 with legacy m_pTitle (resolved in
    // Linking by MP_TITLE id).
    cStatic* m_pTitle = nullptr;
};

}  // namespace mxh::ui
