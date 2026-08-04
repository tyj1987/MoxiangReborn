// titanrecalldlg.hpp — modern port of 墨香
// CTitanRecallDlg (titan recall progress bar dialog:
// CProgressBarDlg subclass).
//
// 1:1 port of legacy `CTitanRecallDlg` from
//   `墨香【源码】\[Client]MH\TitanRecallDlg.h`
//   and `墨香【源码】\[Client]MH\TitanRecallDlg.cpp`.
//
// What the legacy does:
//   - Ctor: m_bSuccessRecall = FALSE.
//   - Dtor: empty body.
//   - Linking: resolve CObjectGuagen
//     (m_pProgressGuagen by TITAN_RECALL_GUAGE) +
//     1 cStatic (m_pRemaintimeStatic by TITAN_RECALL_TIME).
//     SetSuccessTime(7000) (7 sec base time).
//   - Render: if GetSuccessProgress() == TRUE, send
//     MSGBASE MP_TITAN_RECALL_SYN via NETWORK; call
//     InitProgress(); call base CProgressBarDlg::Render().
//   - OnActionEvent: WE_CLOSEWINDOW branch →
//     OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Society)
//     if HERO state == eObjectState_Society. lId ==
//     TITAN_RECALL_CANCEL → send MSGBASE
//     MP_TITAN_RECALL_CANCEL_SYN via NETWORK.
//   - GetSuccessRecall/SetSuccessRecall: getter/setter
//     for m_bSuccessRecall.
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_bSuccessRecall = false
//     via default member init).
//   - Dtor: empty.
//   - Linking: REAL — resolve 2 children by id +
//     SetSuccessTime(7000).
//   - Render: REAL -- on GetSuccessProgress()=true, the
//     host-injected RECALL send callback is invoked
//     (replaces legacy NETWORK send of MSGBASE
//     MP_TITAN/MP_TITAN_RECALL_SYN with HEROID) and
//     InitProgress() fires. With no callback the send
//     is skipped but InitProgress() still fires (1:1
//     with legacy post-send teardown).
//   - OnActionEvent: REAL -- WE_CLOSEWINDOW branch uses
//     host OBJECTSTATEMGR + HERO callbacks (legacy:
//     EndObjectState(HERO, eObjectState_Society) when
//     HERO state == eObjectState_Society). kIdCancelBtn
//     branch uses host RECALL_CANCEL_SYN send callback
//     (legacy: MSGBASE MP_TITAN/MP_TITAN_RECALL_CANCEL_SYN
//     with HEROID). Unknown lIds and non-SUCCESS we codes
//     are safe no-ops. Returns TRUE (1:1 with legacy).
//   - GetSuccessRecall/SetSuccessRecall: REAL.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is a Tier 2 dialog port (after
// cUniqueItemMixProgressBarDlg). The dialog has 2
// children (1 CObjectGuagen + 1 cStatic) + 1 state
// field m_bSuccessRecall. HERO + OBJECTSTATEMGR +
// NETWORK are reached through OPTIONAL host-injected
// callbacks (SetRecallSendCallbacks + SetCloseWindowCallbacks)
// rather than the legacy globals, decoupling the dialog
// from framework singletons.

#pragma once

#include "progressbardlg.hpp"

#include <cstdint>

namespace mxh::ui {

class cTitanRecallDlg : public cProgressBarDlg {
public:
    cTitanRecallDlg();
    ~cTitanRecallDlg() override;

    // ----- 1:1 with legacy CTitanRecallDlg::Linking -----

    // 1:1 with legacy Linking. Resolve 1
    // CObjectGuagen (m_pProgressGuagen by
    // kIdProgressBarGage) + 1 cStatic
    // (m_pRemaintimeStatic by kIdRemaintimeTime) +
    // SetSuccessTime(kBaseSuccessTime=7000).
    void Linking();

    // ----- 1:1 with legacy CTitanRecallDlg::Render override -----

    // 1:1 with legacy Render override. On
    // GetSuccessProgress()=true, the RECALL send
    // callback (SetRecallSendCallbacks) is invoked
    // (replaces legacy NETWORK send) and InitProgress()
    // is called. With no callback registered the send
    // is skipped but InitProgress() still fires (1:1
    // with legacy post-send teardown).
    void Render() override;

    // ----- 1:1 with legacy CTitanRecallDlg::OnActionEvent -----

    // 1:1 with legacy OnActionEvent. The WE_CLOSEWINDOW
    // branch uses SetCloseWindowCallbacks to read the
    // hero state and EndObjectState (eObjectState_Society=24)
    // when in society state. The kIdCancelBtn branch uses
    // SetRecallSendCallbacks to send the legacy cancel
    // MSGBASE. Other lIds/we codes are no-ops. Returns
    // TRUE (1:1 with legacy).
    bool OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- 1:1 with legacy CTitanRecallDlg::GetSuccessRecall -----

    // 1:1 with legacy GetSuccessRecall. Returns
    // m_bSuccessRecall.
    bool GetSuccessRecall() const noexcept {
        return m_bSuccessRecall;
    }

    // ----- 1:1 with legacy CTitanRecallDlg::SetSuccessRecall -----

    // 1:1 with legacy SetSuccessRecall(BOOL bVal).
    // Sets m_bSuccessRecall.
    void SetSuccessRecall(bool bVal) noexcept {
        m_bSuccessRecall = bVal;
    }

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h TITAN_RECALL_GUAGE /
    // TITAN_RECALL_TIME / TITAN_RECALL_CANCEL.
    // Local 690-692.
    static constexpr std::int32_t kIdProgressBarGage  = 690;
    static constexpr std::int32_t kIdRemaintimeTime   = 691;
    static constexpr std::int32_t kIdCancelBtn        = 692;

    // 1:1 with legacy SetSuccessTime(7000) (7 sec
    // base time).
    static constexpr std::uint32_t kBaseSuccessTime = 7000;

    // 1:1 with legacy eObjectState_Society = 24 (1:1 with
    // CommonGameDefine.h enum eObjectState). The hero
    // GetHeroStateFn returns this value when the hero is in
    // a society state; EndObjectState is invoked with it.
    static constexpr std::int32_t kObjectStateSociety = 24;

    // 1:1 with legacy WE_CLOSEWINDOW (=1 in clientMH cWindowDef.h).
    static constexpr std::uint32_t kWeCloseWindow = 1;

    // 1:1 with legacy MP_TITAN category (MP_CATEGORY enum offset).
    static constexpr std::uint8_t kTitanCategory = 72;

    // 1:1 with legacy MP_TITAN_RECALL_SYN (MP_PROTOCOL_TITAN enum).
    static constexpr std::uint8_t kTitanRecallSynProtocol = 3;

    // 1:1 with legacy MP_TITAN_RECALL_CANCEL_SYN.
    static constexpr std::uint8_t kTitanRecallCancelSynProtocol = 6;

    // ----- Host-injected callbacks (legacy: HERO + OBJECTSTATEMGR + NETWORK singletons) -----

    using GetHeroObjectIdFn = std::uint32_t (*)(void* userData);
    using SendRecallSynFn =
        bool (*)(std::uint32_t objectId, void* userData);
    using SendRecallCancelSynFn =
        bool (*)(std::uint32_t objectId, void* userData);
    using GetHeroStateFn = std::int32_t (*)(void* userData);
    using EndObjectStateFn =
        void (*)(std::uint32_t objectId, std::int32_t stateIdx,
                 void* userData);

    // Bundle for both the Render success path and the kIdCancelBtn
    // branch in OnActionEvent. The dialog knows the protocol offsets
    // (kTitanRecallSynProtocol vs kTitanRecallCancelSynProtocol);
    // the host just sees objectId and returns success/failure.
    void SetRecallSendCallbacks(GetHeroObjectIdFn getHeroObjectId,
                                SendRecallSynFn sendRecallSyn,
                                SendRecallCancelSynFn sendRecallCancelSyn,
                                void* userData = nullptr) noexcept;

    // Bundle for the WE_CLOSEWINDOW branch in OnActionEvent. The
    // dialog queries the host hero state and (when society) ends
    // that state through the host EndObjectState callback.
    void SetCloseWindowCallbacks(GetHeroObjectIdFn getHeroObjectId,
                                 GetHeroStateFn getHeroState,
                                 EndObjectStateFn endObjectState,
                                 void* userData = nullptr) noexcept;

private:
    // 1:1 with legacy m_bSuccessRecall (BOOL, init FALSE).
    bool m_bSuccessRecall = false;

    GetHeroObjectIdFn m_getHeroObjectIdFn = nullptr;
    SendRecallSynFn m_sendRecallSynFn = nullptr;
    SendRecallCancelSynFn m_sendRecallCancelSynFn = nullptr;
    GetHeroStateFn m_getHeroStateFn = nullptr;
    EndObjectStateFn m_endObjectStateFn = nullptr;
    void* m_recallUserData = nullptr;
    void* m_closeWindowUserData = nullptr;
};

}  // namespace mxh::ui
