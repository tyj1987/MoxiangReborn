// wanteddialog.hpp — modern port of 墨香 CWantedDialog
// (wanted list dialog: 1 cListDialog).
//
// 1:1 port of legacy `CWantedDialog` from
//   `墨香【源码】\[Client]MH\WantedDialog.h` (759 B) and
//   `墨香【源码】\[Client]MH\WantedDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_WANTEDDIALOG (legacy cWindow
//     type tag).
//   - Dtor: empty body.
//   - Linking: resolve 1 cListDialog (m_pWantedLDG
//     by QUE_WANTEDLDLG id).
//   - SetInfo(WANTEDLIST* pInfo): call InitWanted;
//     for i in [0, MAX_WANTED_NUM): if pInfo[i].
//     WantedIDX == 0 → break; AddItem(pInfo[i].
//     RegistDate, 0xffffffff); sprintf temp =
//     CHATMGR->GetChatMsg(545) with pInfo[i].
//     WantedName; AddItem(temp, 0xffffffff); then
//     ResetGuageBarPos().
//   - AddInfo(WANTEDLIST* pInfo): AddItem(pInfo->
//     RegistDate, 0xffffffff); sprintf temp =
//     CHATMGR->GetChatMsg(545) with pInfo->
//     WantedName; AddItem(temp, 0xffffffff).
//   - InitWanted: m_pWantedLDG->RemoveAll().
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_type = WT_WANTEDDIALOG
//     drop, modern cWindow does not have m_type).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve cListDialog child by
//     id.
//   - SetInfo: TODO (WANTEDLIST struct not ported
//     + CHATMGR not ported, R-12.x deferred). The
//     modern port calls InitWanted (REAL) but
//     the loop body is TODO. When ported, the
//     body becomes the legacy code.
//   - AddInfo: TODO (same as SetInfo).
//   - InitWanted: REAL — call RemoveAll on
//     cListDialog.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 37th **Tier 2** dialog port (after
// cReinforceDataGuideDlg). The dialog has no
// service dependency on the modern service
// interface (Phase 13) — only CHATMGR singleton
// + WANTEDLIST struct (R-12.x deferred).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cListDialog;

class cWantedDialog : public cDialog {
public:
    cWantedDialog();
    ~cWantedDialog() override;

    // ----- 1:1 with legacy CWantedDialog::Linking -----

    // 1:1 with legacy Linking. Resolve cListDialog
    // child (m_pWantedLDG by kIdWantedList) by id.
    void Linking();

    // ----- 1:1 with legacy CWantedDialog::SetInfo -----

    // 1:1 with legacy SetInfo(WANTEDLIST* pInfo).
    // Calls InitWanted (REAL) + the for-loop is
    // TODO (WANTEDLIST struct + CHATMGR not ported,
    // R-12.x deferred). When ported, the body
    // becomes the legacy code with MAX_WANTED_NUM
    // constant + WANTEDLIST struct deref.
    void SetInfo();

    // ----- 1:1 with legacy CWantedDialog::AddInfo -----

    // 1:1 with legacy AddInfo(WANTEDLIST* pInfo).
    // TODO (WANTEDLIST struct + CHATMGR not ported,
    // R-12.x deferred). When ported, the body
    // becomes the legacy code.
    void AddInfo();

    // ----- 1:1 with legacy CWantedDialog::InitWanted -----

    // 1:1 with legacy InitWanted. Call RemoveAll
    // on cListDialog. REAL (no singleton
    // dependency).
    void InitWanted();

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h WINDOW_ID
    // (QUE_WANTEDLDLG). Local 500 — distinct
    // from 200-498 used by previous Tier 2
    // dialogs.
    static constexpr std::int32_t kIdWantedList = 500;

    // MAX_WANTED_NUM (1:1 with legacy common
    // header constant). Used by SetInfo to
    // determine the loop bound.
    static constexpr std::int32_t kMaxWantedNum = 20;

private:
    // 1:1 with legacy m_pWantedLDG (resolved in
    // Linking by QUE_WANTEDLDLG id).
    cListDialog* m_pWantedLDG = nullptr;
};

}  // namespace mxh::ui
