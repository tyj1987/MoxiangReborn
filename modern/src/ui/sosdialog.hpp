// sosdialog.hpp — modern port of 墨香 CSOSDlg (guild SOS
// dialog: list of guild members to send SOS to when in
// trouble).
//
// 1:1 port of legacy `CSOSDlg` from
//   `墨香【源码】\[Client]MH\SOSDialog.h` (641 B) and
//   `墨香【源码】\[Client]MH\SOSDialog.cpp`.
//
// What the legacy does:
//   - Linking() resolves 1 cListDialog (m_pListDlg) and
//     1 cButton (m_pSOSOkBtn), calls
//     m_pListDlg->SetShowSelect(TRUE) +
//     SetHeight(158).
//   - SetActive(val) override: SOSMemberInfo() (fetch
//     guild member list via GUILDMGR + populate
//     m_pListDlg) → cDialog::SetActive(val) → if
//     !val, send MP_GUILD_SOS_SEND_CANCEL to server.
//   - ActionEvent(mouseInfo) override: cDialog's
//     ActionEvent + track m_dwSelectIdx from clicked row
//     in m_pListDlg.
//   - SOSMemberInfo() helper: pulls GUILDMGR member list,
//     formats name + rank + level, color-codes based on
//     bLogged (online gray, offline lighter).
//   - OnActionEvent(lId, p, we) handles SOS_OKBTN:
//     fetch the selected member → if MemberIdx == HEROID
//     chat msg 1631 + break; if bLogged == FALSE chat
//     msg 1632 + break; else build MSG_DWORD4 (SOS
//     request: member id + map num + move point +
//     channel) and send via NETWORK.
//   - ~CSOSDlg calls m_pListDlg->RemoveAll() (1:1 quirk:
//     nil-deref if Linking never resolved m_pListDlg;
//     modern port is null-checked).
//
// The modern port covers everything that doesn't need
// a singleton: Linking (real — pure widget ops on
// cListDialog + cButton), SetActive (no-op SOSMemberInfo
// fetch + no-op cancel send), ActionEvent (no-op row
// select), OnActionEvent (no-op SOS send), ~CSOSDlg
// (null-checked RemoveAll).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md), this
// is the 6th **Tier 2** dialog port (after cExitDialog,
// cMacroDialog, cCharMakeDlg, cGuildJoinDialog,
// cCharStateDialog). The dialog has no service dependency
// on the modern service interface (Phase 13) — all state
// lives in 5 global singletons (HEROID / NETWORK /
// GUILDMGR / MAP / CHATMGR / WINDOWMGR), none of which
// are ported yet. The GuildManager port is tracked as a
// future Tier 3 work item (depends on PlayerStatsService
// already in Phase 13.2).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cListDialog;
class cButton;

class cSOSDialog : public cDialog {
public:
    cSOSDialog();
    ~cSOSDialog() override;

    // ----- 1:1 with legacy CSOSDlg::Linking -----

    // Resolves 1 cListDialog (m_pListDlg) and 1 cButton
    // (m_pSOSOkBtn), calls m_pListDlg->SetShowSelect(TRUE)
    // + SetHeight(158). REAL — no singleton.
    void Linking();

    // ----- 1:1 with legacy CSOSDlg::SetActive -----

    // 1:1 override: legacy calls SOSMemberInfo() (fetch
    // guild member list via GUILDMGR + populate
    // m_pListDlg) then cDialog::SetActive(val) then if
    // !val send MP_GUILD_SOS_SEND_CANCEL. Modern port
    // calls base SetActive(val) (the SOSMemberInfo fetch
    // and cancel send are no-op stubs until GUILDMGR +
    // NETWORK are ported).
    void SetActive(bool val) noexcept override;

    // ----- 1:1 with legacy CSOSDlg::ActionEvent -----

    // 1:1 override: legacy calls cDialog::ActionEvent +
    // tracks m_dwSelectIdx from clicked row in
    // m_pListDlg. Modern port calls base ActionEvent
    // (row-click tracking is no-op until WINDOWMGR +
    // cListDialog::PtIdxInRow are ported).
    std::uint32_t ActionEvent(std::int32_t mouseX,
                              std::int32_t mouseY,
                              std::uint32_t mouseFlags) override;

    // ----- 1:1 with legacy CSOSDlg::SOSMemberInfo -----

    // Fetch guild member list + populate m_pListDlg
    // with formatted "name rank level" rows. The legacy
    // pulls GUILDMGR->GetGuild()->GetMemberList() +
    // formats each member's name (padded with 0x20) +
    // rank (padded) + level + color-codes based on
    // bLogged. Modern port is a no-op until GUILDMGR is
    // ported. The "remove all" + "add item" surface
    // (cListDialog API) is already in place.
    void SOSMemberInfo();

    // ----- 1:1 with legacy CSOSDlg::OnActionEvent -----

    // Dispatch a button click. The legacy handles
    // SOS_OKBTN: fetch the selected member → if
    // MemberIdx == HEROID chat msg 1631 + break; if
    // bLogged == FALSE chat msg 1632 + break; else
    // build MSG_DWORD4 (SOS request) and send via
    // NETWORK. Modern port is a no-op until GUILDMGR +
    // HEROID + MAP + CHATMGR + NETWORK singletons are
    // ported.
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- Accessors (used by tests + future singleton bridge) -----

    cListDialog* GetMemberList() const noexcept { return m_pListDlg; }
    cButton*     GetOkButton()   const noexcept { return m_pSOSOkBtn; }
    std::uint32_t GetSelectIdx() const noexcept { return m_dwSelectIdx; }
    void         SetSelectIdx(std::uint32_t idx) noexcept { m_dwSelectIdx = idx; }

    // ----- Local id range (matches modern test convention) -----

    static constexpr std::int32_t kMemberListId = 230;  // was SOS_MEMBERLIST
    static constexpr std::int32_t kOkBtnId      = 231;  // was SOS_OKBTN

private:
    cListDialog* m_pListDlg    = nullptr;
    cButton*     m_pSOSOkBtn   = nullptr;
    std::uint32_t m_dwSelectIdx = 0;
};

}  // namespace mxh::ui
