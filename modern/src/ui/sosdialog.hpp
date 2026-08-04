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
// lives in legacy singletons; modern adapters provide
// their values without coupling the UI library to them.

#pragma once

#include "cdialog.hpp"

#include <cstddef>
#include <cstdint>

namespace mxh::ui {

class cListDialog;
class cButton;

struct SOSGuildMember {
    std::uint32_t memberIdx = 0;
    const char* memberName = nullptr;
    const char* rankName = nullptr;
    std::int32_t level = 0;
    bool logged = false;
};

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

    using GetMemberCountFn = std::size_t (*)(void* userData);
    using GetMemberFn = bool (*)(std::size_t index, SOSGuildMember* member,
                                 void* userData);
    using GetHeroObjectIdFn = std::uint32_t (*)(void* userData);
    using GetDwordFn = std::uint32_t (*)(void* userData);
    using GetPositionFn = void (*)(float* x, float* z, void* userData);
    using AddSystemMessageFn = void (*)(std::int32_t messageId,
                                        void* userData);
    using SendCancelFn = void (*)(std::uint32_t objectId, void* userData);
    using SendSOSFn = void (*)(std::uint32_t objectId,
                               std::uint32_t memberId,
                               std::uint32_t mapNum,
                               std::uint32_t movePoint,
                               std::uint32_t channel,
                               void* userData);
    using IsMouseDownUsedFn = bool (*)(void* userData);

    void SetCallbacks(GetMemberCountFn getMemberCount,
                      GetMemberFn getMember,
                      GetHeroObjectIdFn getHeroObjectId,
                      GetDwordFn getMapNum,
                      GetDwordFn getChannelNum,
                      GetPositionFn getHeroPosition,
                      AddSystemMessageFn addSystemMessage,
                      SendCancelFn sendCancel,
                      SendSOSFn sendSOS,
                      IsMouseDownUsedFn isMouseDownUsed,
                      void* userData = nullptr) noexcept;

    // 1:1 override: legacy calls SOSMemberInfo() (fetch
    // guild member list via GUILDMGR + populate
    // m_pListDlg) then cDialog::SetActive(val) then if
    // !val send MP_GUILD_SOS_SEND_CANCEL. Modern host
    // callbacks supply both guild refresh and cancel send.
    void SetActive(bool val) noexcept override;

    // ----- 1:1 with legacy CSOSDlg::ActionEvent -----

    // 1:1 override: legacy calls cDialog::ActionEvent +
    // tracks m_dwSelectIdx from clicked row in
    // m_pListDlg. Modern port uses cListDialog hit-test
    // plus the host mouse-consumed state.
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
    static constexpr std::uint32_t kWeLeftButtonClick = 0x0002u;
    static constexpr std::int32_t kSelfTargetMessageId = 1631;
    static constexpr std::int32_t kOfflineTargetMessageId = 1632;
    static constexpr std::uint32_t kOnlineColor = 0xFFFFFFFFu;
    static constexpr std::uint32_t kOfflineColor = 0xFFACB6C7u;

private:
    GetMemberCountFn m_getMemberCount = nullptr;
    GetMemberFn m_getMember = nullptr;
    GetHeroObjectIdFn m_getHeroObjectId = nullptr;
    GetDwordFn m_getMapNum = nullptr;
    GetDwordFn m_getChannelNum = nullptr;
    GetPositionFn m_getHeroPosition = nullptr;
    AddSystemMessageFn m_addSystemMessage = nullptr;
    SendCancelFn m_sendCancel = nullptr;
    SendSOSFn m_sendSOS = nullptr;
    IsMouseDownUsedFn m_isMouseDownUsed = nullptr;
    void* m_callbackUserData = nullptr;

    cListDialog* m_pListDlg    = nullptr;
    cButton*     m_pSOSOkBtn   = nullptr;
    std::uint32_t m_dwSelectIdx = 0;
};

}  // namespace mxh::ui
