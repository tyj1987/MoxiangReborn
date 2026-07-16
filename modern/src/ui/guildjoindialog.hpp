// guildjoindialog.hpp — modern port of 墨香 CGuildJoinDialog
// (guild member-invite dialog: "invite player to guild" or
// "invite player as student member").
//
// 1:1 port of legacy `CGuildJoinDialog` from
//   `墨香【源码】\[Client]MH\GuildJoinDialog.h` (275 B) and
//   `墨香【源码】\[Client]MH\GuildJoinDialog.cpp`.
//
// What the legacy does:
//   - Linking() is empty in the legacy (1:1 quirk: the
//     legacy Linking is a no-op; the dialog is purely
//     dispatch-driven from OnActionEvent).
//   - OnActionEvent(lId, p, we) dispatches 3 button ids:
//       JO_MEMBERBTN: rank check (HERO->GetGuildMemberRank() <
//                     GUILD_VICEMASTER → chat msg 297 + return)
//                     → OBJECTMGR->GetSelectedObject() must be
//                     a player with no guild → GUILDMGR->
//                     AddMemberSyn(target.id)
//       JO_STUDENTBTN: guild-level check (GUILDMGR->GetLevel()
//                     < GUILD_5LEVEL → chat msg 1368 + return)
//                     → target must be a guildless player →
//                     GUILDMGR->AddStudentSyn(target.id,
//                     target.level)
//       JO_CANCELBTN: legacy's SetActive(FALSE) is commented
//                     out (1:1 quirk: Korean dev comment says
//                     "暂放弃，改默认 CANCEL" — modern port
//                     keeps the comment-out behavior).
//   - After all 3 branches: SetActive(FALSE) closes the
//     dialog. The default branch in the switch calls
//     ASSERT(0) — the modern port treats unknown ids as a
//     no-op (no assert, since the modern test surface does
//     not run an assert harness).
//
// The modern port covers the dialog structure (Linking +
// OnActionEvent) but does NOT port GuildManager /
// ObjectManager / ChatManager / Hero / Player. Those are
// global singletons that the legacy dispatches to. Until
// they are ported (via the modern service interface from
// Phase 13 or directly), OnActionEvent is a no-op that
// preserves the *state-machine shape* of the legacy
// switch (the 3 button ids are still distinguished so a
// future port can wire the dispatch without breaking the
// public API). The dialog itself is still testable
// through:
//   - Default construction
//   - Linking (no-op, doesn't crash, leaves no observable
//     state)
//   - OnActionEvent with each of the 3 button ids (no-op,
//     no crash, no observable state change)
//   - OnActionEvent with an unknown id (no-op, no assert)
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this
// is the 4th **Tier 2** dialog port (after cExitDialog,
// cMacroDialog, cCharMakeDlg). The dialog has no service
// dependency on the modern service interface (Phase 13)
// — all state lives in 4 global singletons (HERO /
// OBJECTMGR / CHATMGR / GUILDMGR), none of which are
// ported yet. The GuildManager port is tracked as a
// future Tier 3 work item (depends on PlayerStatsService
// already in Phase 13.2).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cGuildJoinDialog : public cDialog {
public:
    cGuildJoinDialog();
    ~cGuildJoinDialog() override;

    // ----- 1:1 with legacy CGuildJoinDialog::Linking -----

    // 1:1 quirk: the legacy Linking() is empty. The dialog
    // is purely dispatch-driven from OnActionEvent. Modern
    // port mirrors the empty body.
    void Linking();

    // ----- 1:1 with legacy CGuildJoinDialog::OnActionEvent -----

    // Dispatch a button click. The legacy switch handles 3
    // button ids (JO_MEMBERBTN / JO_STUDENTBTN /
    // JO_CANCELBTN) and falls through to ASSERT(0) on
    // unknown ids. The modern port preserves the
    // state-machine shape (the 3 ids are distinguished)
    // but the body is a no-op until GuildManager +
    // ObjectManager + ChatManager + Hero + Player are
    // ported. See the TODO in guildjoindialog.cpp for the
    // exact dispatch logic that will be wired when those
    // singletons are ported.
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- Local id range (matches modern test convention;
    //       the legacy JO_* ids live in WindowIDs.h, not
    //       yet ported). See cListDialogEx + cCharMakeDlg
    //       for the same pattern. -----

    static constexpr std::int32_t kJoinMemberBtnId   = 210;  // was JO_MEMBERBTN
    static constexpr std::int32_t kJoinStudentBtnId  = 211;  // was JO_STUDENTBTN
    static constexpr std::int32_t kJoinCancelBtnId   = 212;  // was JO_CANCELBTN
};

}  // namespace mxh::ui
