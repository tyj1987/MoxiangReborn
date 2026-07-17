// guildinvitationkindselectiondialog.hpp — modern port of 墨香
// CGuildInvitationKindSelectionDialog (guild invitation kind
// selector dialog: 3 button — "invite as member" / "invite as
// student" / "cancel").
//
// 1:1 port of legacy
// `CGuildInvitationKindSelectionDialog` from
//   `墨香【源码】\[Client]MH\GuildInvitationKindSelectionDialog.h`
//   (329 B) and
//   `墨香【源码】\[Client]MH\GuildInvitationKindSelectionDialog.cpp`.
//
// What the legacy does:
//   - Ctor / dtor: empty bodies (no state init).
//   - Linking: empty body. The dialog is purely
//     dispatch-driven from OnActionEvent.
//   - OnActionEvent: 3 button ids:
//     * JO_MEMBERBTN: 3-singleton dispatch
//       (OBJECTMGR->GetSelectedObject → if player
//       and no guild → GUILDMGR->AddMemberSyn).
//     * JO_STUDENTBTN: 3-singleton dispatch
//       (GUILDMGR->GetLevel() < GUILD_5LEVEL → chat
//       msg 1368 + return; else
//       GUILDMGR->AddStudentSyn).
//     * JO_CANCELBTN: SetActive(FALSE) is commented
//       out in the legacy (1:1 quirk: Korean dev
//       comment says "暂放弃，改默认 CANCEL" — modern
//       port keeps the comment-out behavior, no
//       SetActive).
//   - After all 3 branches: SetActive(FALSE) closes
//     the dialog. The default branch in the switch
//     calls ASSERT(0) — the modern port treats
//     unknown ids as a no-op (no assert, since the
//     modern test surface does not run an assert
//     harness).
//
// The modern port covers the dialog structure
// (Linking + OnActionEvent) but does NOT port
// GuildManager / ObjectManager / ChatManager /
// Hero / Player. Those are global singletons
// (R-12.x deferred). OnActionEvent is a no-op that
// preserves the *state-machine shape* of the
// legacy (3-button dispatch, post-branch
// SetActive(FALSE) for MEMBER+STUDENT, no-op for
// CANCEL).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 24th **Tier 2** dialog port (after
// cNameChangeNotifyDlg). The dialog has no service
// dependency on the modern service interface
// (Phase 13) — all state lives in 3 global
// singletons.

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cGuildInvitationKindSelectionDialog : public cDialog {
public:
    cGuildInvitationKindSelectionDialog();
    ~cGuildInvitationKindSelectionDialog() override;

    // ----- 1:1 with legacy CGuildInvitationKindSelectionDialog::Linking -----

    // 1:1 with legacy: empty body. The dialog is
    // purely dispatch-driven from OnActionEvent
    // (no children to resolve).
    void Linking() noexcept {}

    // ----- 1:1 with legacy CGuildInvitationKindSelectionDialog::OnActionEvent -----

    // 1:1 with legacy 3-button dispatch. All 3
    // branches are TODO (3-singleton: OBJECTMGR +
    // CHATMGR + GUILDMGR not ported, R-12.x
    // deferred). After all 3 branches, the legacy
    // calls SetActive(FALSE) — the modern port
    // documents this as TODO (GUILDMGR
    // dispatch).
    //
    // 1:1 quirk: legacy CANCEL branch's
    // SetActive(FALSE) is commented out (Korean dev
    // comment). Modern port keeps the comment-out
    // (the CANCEL branch is a complete no-op).
    //
    // 1:1 quirk: legacy default branch calls
    // ASSERT(0). Modern port treats unknown ids as
    // a no-op (no assert).
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h WINDOW_ID values
    // (JO_CANCELBTN / JO_MEMBERBTN / JO_STUDENTBTN).
    // Local 370-372 — distinct from 200-360 used
    // by previous Tier 2 dialogs.
    static constexpr std::int32_t kIdCancelBtn = 370;
    static constexpr std::int32_t kIdMemberBtn = 371;
    static constexpr std::int32_t kIdStudentBtn = 372;
};

}  // namespace mxh::ui
