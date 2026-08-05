// 1:1 port of legacy CMNPlayRoomDialog from
//   mnplayroomdialog.h (1,465 B) + mnplayroomdialog.cpp (4,477 B).
//
// MurimNet (PvP server) play-room dialog. Shows three team
// player lists (TeamA / TeamB / Observer), a chat edit + log,
// and a Start button visible only when the player is the
// room captain. Buttons dispatch team-change / exit / start
// requests to host callbacks (the legacy called NETWORK->
// Send with MURIMNET singletons; the modern port defers the
// network layer and uses injected callbacks).
//
// Children (1:1 with legacy MNPRI_*):
//   - 3 cListDialog (team A/B/observer player rosters)
//   - 1 cButton (start - captain only)
//   - 1 cEditBox (chat input)
//   - 1 cListDialog (chat log)
//   - 1 cStatic (room title)
//
// 1:1 quirk: MoveToOB button dispatches but the legacy line is
// commented out (no SendMsgTeamChange(2)). Modern port follows
// the same shape - move-to-OB is a button id that fires no callback.


#pragma once
#include "cdialog.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
namespace mxh::ui {

class cButton;
class cEditBox;
class cListDialog;
class cStatic;

// 1:1 with legacy eTEAM_* enum used by MNPlayRoomDialog.cpp
// for player-roster indexing (m_pPlayerListDlg indexed by team).
// The values match legacy BYTE ordering 0/1/2 = Left/Right/Observer,
// Max=3. Per AGENTS.md 1:1 contract preserved rule (legacy header
// constants may not be modified).
enum class PlayRoomTeam : std::int32_t {
    TeamA    = 0,  // eTEAM_LEFT
    TeamB    = 1,  // eTEAM_RIGHT
    Observer = 2,  // eTEAM_OBSERVER
    Max      = 3,  // eTEAM_MAX
};

// 1:1 with legacy PRCTC_* chat-class enum. Legacy had three
// values: PRCTC_WHOLE (0), PRCTC_TEAM (1), PRCTC_WHISPER (2).
// ChatMsg only formats the WHOLE branch (legacy switch only
// implements case PRCTC_WHOLE). The other values exist for
// 1:1 reference but the modern ChatMsg formats them all as
// the same [name]: msg line (matches legacy PrintMsg fallback).
enum class PlayRoomChatClass : std::int32_t {
    Whole   = 0,  // PRCTC_WHOLE
    Team    = 1,  // PRCTC_TEAM (not implemented in legacy body)
    Whisper = 2,  // PRCTC_WHISPER (not implemented in legacy body)
};

// 1:1 with legacy MNPLAYER_BASEINFO layout (name string +
// team BYTE + level DWORD). Modern port uses std::string for
// name (legacy was char[17]) and std::uint16_t for level (legacy
// was DWORD, but stored a level value <= 200).
struct MNPlayerInfo {
    std::string        name;   // legacy char strPlayerName[MAX_NAME_LENGTH+1]
    std::uint16_t      level;  // legacy DWORD
    PlayRoomTeam       team;   // legacy BYTE cbTeam
};

// 1:1 with legacy PLAYROOM_BASEINFO struct fields used by
// MNPlayRoomDialog::SetPlayRoomInfo. Modern port carries the
// five fields the legacy assigns: title (char[128]) + max
// (WORD) + game kind (WORD) + room kind (WORD) + repay (DWORD).
struct MNPlayRoomInfo {
    std::string   title;        // legacy char strPlayRoomTitle[128]
    std::uint16_t max_players;  // legacy WORD
    std::uint16_t game_kind;    // legacy WORD
    std::uint16_t room_kind;    // legacy WORD
    std::uint32_t repay;        // legacy DWORD
};


// 1:1 with legacy room-action dispatch. The legacy dialog
// calls MURIMNET->GetMNPlayerManager()->GetMNHeroID() then
// NETWORK->Send with category MP_MURIMNET. Modern port
// does not link MurimNet singletons yet (R-12.x deferred);
// instead, button clicks route through injected callbacks.
// Defaults are no-ops so a dialog without a host callback set
// behaves like an unconnected legacy instance (button clicks
// are silently dropped, matching the pre-network-init state).
using TeamChangeCallback  = std::function<void(PlayRoomTeam)>;
using ExitRequestCallback = std::function<void()>;
using StartRequestCallback = std::function<void()>;
using ChatSubmitCallback   = std::function<void(const std::string&)>;


class cMNPlayRoomDialog : public cDialog {
public:
    static constexpr int kNumTeams = 3;  // 1:1 with eTEAM_MAX

    // Local id range (1:1 with legacy MNPRI_* window ids, rebased
    // to 730..739 to avoid conflicts with previously-ported dialogs).
    static constexpr int kIdBtnMoveToA  = 730;
    static constexpr int kIdBtnMoveToB  = 731;
    static constexpr int kIdBtnMoveToOB = 732;
    static constexpr int kIdBtnExit     = 733;
    static constexpr int kIdBtnStart    = 734;
    static constexpr int kIdListTeamA   = 735;
    static constexpr int kIdListTeamB   = 736;
    static constexpr int kIdListObs     = 737;
    static constexpr int kIdEdtChat     = 738;
    static constexpr int kIdListChat    = 739;
    static constexpr int kIdStcTitle    = 740;

    cMNPlayRoomDialog();
    ~cMNPlayRoomDialog() override;

    void Linking();
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // Callback injection (replaces legacy NETWORK->Send calls in
    // SendMsgTeamChange / SendMsgExit / SendMsgStart). Default = no-op.
    void SetTeamChangeCallback(TeamChangeCallback cb) noexcept  { m_teamchange_cb = std::move(cb); }
    void SetExitRequestCallback(ExitRequestCallback cb) noexcept { m_exit_cb = std::move(cb); }
    void SetStartRequestCallback(StartRequestCallback cb) noexcept { m_start_cb = std::move(cb); }
    void SetChatSubmitCallback(ChatSubmitCallback cb) noexcept { m_chat_cb = std::move(cb); }
    void ClearCallbacks() noexcept {
        m_teamchange_cb = nullptr; m_exit_cb = nullptr;
        m_start_cb = nullptr; m_chat_cb = nullptr;
    }


    // 1:1 with legacy AddPlayer / RemovePlayer / RemoveAllPlayer
    // (each manipulates cListDialog.AddItem / RemoveItem / RemoveAll
    // on the matching team list).
    void AddPlayer(const MNPlayerInfo& player);
    void RemovePlayer(const std::string& playerName, PlayRoomTeam team);
    void RemoveAllPlayer();
    void TeamChange(const std::string& playerName,
                    PlayRoomTeam fromTeam,
                    PlayRoomTeam toTeam);

    void SetCaptain(bool isCaptain);
    bool IsCaptain() const noexcept { return m_is_captain; }

    void SetPlayRoomInfo(const MNPlayRoomInfo& info);
    const MNPlayRoomInfo& GetPlayRoomInfo() const noexcept { return m_info; }

    void ChatMsg(PlayRoomChatClass nClass,
                 const std::string& strName,
                 const std::string& strMsg);
    void PrintMsg(PlayRoomChatClass nClass, const std::string& str);
    std::size_t ChatHistorySize() const noexcept { return m_chat_history.size(); }
    const std::string& GetChatLine(std::size_t i) const;

    // Player roster inspection (mirrors legacy cListDialog RowCount for
    // each team roster).
    std::size_t PlayerCount(PlayRoomTeam team) const;
    const std::string& PlayerAt(PlayRoomTeam team, std::size_t i) const;

    // Test accessors.
    const cListDialog* GetListTeamA()  const noexcept { return m_pListTeams[static_cast<std::size_t>(PlayRoomTeam::TeamA)].get(); }
    const cListDialog* GetListTeamB()  const noexcept { return m_pListTeams[static_cast<std::size_t>(PlayRoomTeam::TeamB)].get(); }
    const cListDialog* GetListObs()    const noexcept { return m_pListTeams[static_cast<std::size_t>(PlayRoomTeam::Observer)].get(); }
    const cButton*     GetBtnStart()   const noexcept { return m_pBtnStart.get(); }
    const cEditBox*    GetChatEdit()   const noexcept { return m_pEdtChat.get(); }
    const cListDialog* GetChatList()   const noexcept { return m_pLstChat.get(); }
    const cStatic*     GetTitle()      const noexcept { return m_pTitle.get(); }

    bool HasCallbackSet() const noexcept {
        return static_cast<bool>(m_teamchange_cb) ||
               static_cast<bool>(m_exit_cb) ||
               static_cast<bool>(m_start_cb) ||
               static_cast<bool>(m_chat_cb);
    }

private:
    // 1:1 quirk: legacy cListDialog* m_pPlayerListDlg[eTEAM_MAX]
    // (raw), modern unique_ptr-as-member so tests can inspect without
    // ownership aliasing.
    std::array<std::unique_ptr<cListDialog>, static_cast<std::size_t>(PlayRoomTeam::Max)> m_pListTeams{};
    std::unique_ptr<cButton>     m_pBtnStart;
    std::unique_ptr<cEditBox>    m_pEdtChat;
    std::unique_ptr<cListDialog> m_pLstChat;
    std::unique_ptr<cStatic>     m_pTitle;

    // Per-team player rosters (mirrors cListDialog.AddItem content).
    std::array<std::vector<std::string>,
               static_cast<std::size_t>(PlayRoomTeam::Max)> m_teamPlayers{};
    std::vector<std::string> m_chat_history;

    bool m_is_captain = false;
    MNPlayRoomInfo m_info{};

    TeamChangeCallback  m_teamchange_cb;
    ExitRequestCallback m_exit_cb;
    StartRequestCallback m_start_cb;
    ChatSubmitCallback  m_chat_cb;
};

} // namespace mxh::ui

