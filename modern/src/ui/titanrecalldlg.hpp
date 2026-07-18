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
//   - Render: TODO (NETWORK singleton not ported, R-12.x
//     deferred). Modern port calls base Render.
//   - OnActionEvent: TODO (HERO + OBJECTSTATEMGR +
//     NETWORK singletons, R-12.x deferred). Modern port
//     returns TRUE.
//   - GetSuccessRecall/SetSuccessRecall: REAL.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is a Tier 2 dialog port (after
// cUniqueItemMixProgressBarDlg). The dialog has 2
// children (1 CObjectGuagen + 1 cStatic) + 1 state
// field m_bSuccessRecall. HERO + OBJECTSTATEMGR +
// NETWORK are R-12.x deferred.

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

    // 1:1 with legacy Render override. The
    // NETWORK send + InitProgress dispatch is TODO
    // (R-12.x deferred). Modern port calls base
    // Render.
    void Render() override;

    // ----- 1:1 with legacy CTitanRecallDlg::OnActionEvent -----

    // 1:1 with legacy OnActionEvent. The
    // HERO + OBJECTSTATEMGR + NETWORK dispatch is
    // TODO (R-12.x deferred). Modern port returns
    // TRUE (legacy also returns TRUE).
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

private:
    // 1:1 with legacy m_bSuccessRecall (BOOL, init FALSE).
    bool m_bSuccessRecall = false;
};

}  // namespace mxh::ui
