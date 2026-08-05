// cMNPlayRoomDialog implementation. See mnplayroomdialog.hpp for
// 1:1 port rationale + button id map.

#include "mnplayroomdialog.hpp"

#include "cbutton.hpp"
#include "ceditbox.hpp"
#include "clistdialog.hpp"
#include "cstatic.hpp"
#include "legacy_window_event.hpp"

#include <array>
#include <cstdio>
#include <memory>
#include <string>

namespace mxh::ui {

// 1:1 with legacy 0xffffffff color literal used by every cListDialog
// AddItem call (matches ARGB opaque-white).
constexpr std::uint32_t kItemColorWhite = 0xFFFFFFFFu;

cMNPlayRoomDialog::cMNPlayRoomDialog() = default;
cMNPlayRoomDialog::~cMNPlayRoomDialog() = default;

void cMNPlayRoomDialog::Linking() {
    // 1:1 quirk: legacy GetWindowForID; modern make_unique + Init.
    // We own 9 children as unique_ptr members AND drive them via
    // direct member access (the dialog never queries through findWindow).
    {
        auto p = std::make_unique<cListDialog>();
        p->Init(0, 0, 100, 100, nullptr, kIdListTeamA);
        p->InitList(32, 0, 0, 100, 100);
        m_pListTeams[static_cast<std::size_t>(PlayRoomTeam::TeamA)] = std::move(p);
    }
    {
        auto p = std::make_unique<cListDialog>();
        p->Init(0, 0, 100, 100, nullptr, kIdListTeamB);
        p->InitList(32, 0, 0, 100, 100);
        m_pListTeams[static_cast<std::size_t>(PlayRoomTeam::TeamB)] = std::move(p);
    }
    {
        auto p = std::make_unique<cListDialog>();
        p->Init(0, 0, 100, 100, nullptr, kIdListObs);
        p->InitList(32, 0, 0, 100, 100);
        m_pListTeams[static_cast<std::size_t>(PlayRoomTeam::Observer)] = std::move(p);
    }
    {
        auto p = std::make_unique<cButton>();
        p->Init(0, 0, 60, 24, nullptr, nullptr, nullptr, nullptr, nullptr, kIdBtnStart);
        m_pBtnStart = std::move(p);
    }
    {
        auto p = std::make_unique<cEditBox>();
        p->Init(0, 0, 200, 16, nullptr, nullptr, kIdEdtChat);
        m_pEdtChat = std::move(p);
    }
    {
        auto p = std::make_unique<cListDialog>();
        p->Init(0, 0, 200, 100, nullptr, kIdListChat);
        p->InitList(64, 0, 0, 200, 100);
        m_pLstChat = std::move(p);
    }
    {
        auto p = std::make_unique<cStatic>();
        p->Init(0, 0, 200, 16, nullptr, kIdStcTitle);
        m_pTitle = std::move(p);
    }
    // 1:1 quirk: legacy sets Start button off (FALSE) at Linking
    // time (default for non-captain). Modern port honors an existing
    // captain flag set before Linking: the button visibility follows the
    // captain state at Linking time. This preserves the 1:1 contract for
    // the common path (Linking first, then SetCaptain) and avoids losing
    // captain state across Linking.
    if (m_pBtnStart) {
        m_pBtnStart->SetActive(m_is_captain);
    }
}

void cMNPlayRoomDialog::OnActionEvent(std::int32_t lId, void* /*p*/, std::uint32_t we) {
    // 1:1 with legacy OnActionEvent. The legacy dispatches WE_BTNCLICK
    // for MNPRI_BTN_MOVETOA / MOVETOB / MOVETOOB / EXIT / START.
    // Modern port routes through injected callbacks (matches the
    // cmakdial pattern for cCharMakeDlg rotation callbacks).
    //
    // 1:1 quirk: legacy MoveToOB body is commented out (the line
    // SendMsgTeamChange(2) is missing). Modern port follows the same
    // shape - MoveToOB id is recognized but no callback fires.
    if ((we & legacy_window_event::kButtonClick) == 0) return;

    switch (lId) {
    case kIdBtnMoveToA:
        if (m_teamchange_cb) m_teamchange_cb(PlayRoomTeam::TeamA);
        break;
    case kIdBtnMoveToB:
        if (m_teamchange_cb) m_teamchange_cb(PlayRoomTeam::TeamB);
        break;
    case kIdBtnMoveToOB:
        // 1:1 quirk: legacy has no SendMsgTeamChange(2) call here.
        break;
    case kIdBtnExit:
        if (m_exit_cb) m_exit_cb();
        break;
    case kIdBtnStart:
        if (m_start_cb) m_start_cb();
        break;
    default:
        break;
    }
}

void cMNPlayRoomDialog::AddPlayer(const MNPlayerInfo& player) {
    // 1:1 with legacy AddPlayer: cListDialog::AddItem with white color.
    // The legacy passes the raw playerName (no level/format); modern
    // port preserves this (tests check raw name presence in row 0).
    auto idx = static_cast<std::size_t>(player.team);
    if (player.team >= PlayRoomTeam::Max) return;
    m_teamPlayers[idx].push_back(player.name);
    if (m_pListTeams[idx]) {
        m_pListTeams[idx]->AddItem(player.name, kItemColorWhite);
    }
}

void cMNPlayRoomDialog::RemovePlayer(const std::string& playerName, PlayRoomTeam team) {
    // 1:1 with legacy RemovePlayer: removes from the team roster by raw name.
    if (team >= PlayRoomTeam::Max) return;
    auto idx = static_cast<std::size_t>(team);
    auto& roster = m_teamPlayers[idx];
    for (auto it = roster.begin(); it != roster.end(); ++it) {
        if (*it == playerName) {
            roster.erase(it);
            break;
        }
    }
    if (m_pListTeams[idx]) {
        m_pListTeams[idx]->RemoveItem(playerName);
    }
}

void cMNPlayRoomDialog::RemoveAllPlayer() {
    // 1:1 with legacy RemoveAllPlayer: clears each team roster.
    for (std::size_t i = 0; i < m_teamPlayers.size(); ++i) {
        m_teamPlayers[i].clear();
        if (m_pListTeams[i]) m_pListTeams[i]->RemoveAll();
    }
}

void cMNPlayRoomDialog::TeamChange(const std::string& playerName,
                                       PlayRoomTeam fromTeam, PlayRoomTeam toTeam) {
    // 1:1 with legacy TeamChange: removes from fromTeam then adds to toTeam.
    // Legacy passes the raw name; modern port follows.
    if (fromTeam >= PlayRoomTeam::Max || toTeam >= PlayRoomTeam::Max) return;
    if (fromTeam == toTeam) return;  // 1:1 quirk: legacy does not guard same-team, but no-op is harmless
    RemovePlayer(playerName, fromTeam);
    MNPlayerInfo info;
    info.name = playerName;
    info.level = 0;
    info.team = toTeam;
    AddPlayer(info);
}

void cMNPlayRoomDialog::SetCaptain(bool isCaptain) {
    // 1:1 with legacy SetCaptain(BOOL bCaptain):
    //   if (bCaptain) m_pBtnStart->SetActive( TRUE );
    //   else         m_pBtnStart->SetActive( FALSE );
    m_is_captain = isCaptain;
    if (m_pBtnStart) {
        m_pBtnStart->SetActive(isCaptain);
    }
}

void cMNPlayRoomDialog::SetPlayRoomInfo(const MNPlayRoomInfo& info) {
    // 1:1 with legacy SetPlayRoomInfo:
    //   cStatic* pTitle = GetWindowForID(MNPRI_STC_TITLE);
    //   pTitle->SetStaticText(pPlayRoomInfo->strPlayRoomTitle);
    //
    // The legacy only uses the title field; modern port stores all 5
    // fields and shows only the title (matches legacy body). Other
    // fields are kept in m_info so future hosts can render them.
    m_info = info;
    if (m_pTitle && !info.title.empty()) {
        m_pTitle->SetStaticText(info.title);
    }
}

void cMNPlayRoomDialog::ChatMsg(PlayRoomChatClass nClass,
                                 const std::string& strName,
                                 const std::string& strMsg) {
    // 1:1 with legacy ChatMsg. The legacy switch only implements
    // case PRCTC_WHOLE which formats a [name]: msg line and pushes
    // to m_pLstChat. Other branches are absent (the legacy file
    // has no default case either, so other nClass values are silently
    // dropped). Modern port follows the same shape: WHOLE formats
    // + pushes; others are dropped.
    (void)nClass;  // suppress unused if no branch fires
    if (nClass != PlayRoomChatClass::Whole) return;
    std::array<char, 256> buf{};
    std::snprintf(buf.data(), buf.size(), "[%s]: %s", strName.c_str(), strMsg.c_str());
    m_chat_history.emplace_back(buf.data());
    if (m_pLstChat) {
        m_pLstChat->AddItem(std::string(buf.data()), kItemColorWhite);
    }
}

void cMNPlayRoomDialog::PrintMsg(PlayRoomChatClass nClass, const std::string& str) {
    // 1:1 with legacy PrintMsg: adds raw str to chat list (no format).
    // The legacy ignores nClass; modern port is identical.
    (void)nClass;
    m_chat_history.push_back(str);
    if (m_pLstChat) {
        m_pLstChat->AddItem(str, kItemColorWhite);
    }
}

const std::string& cMNPlayRoomDialog::GetChatLine(std::size_t i) const {
    return m_chat_history.at(i);  // bounds-checked
}

std::size_t cMNPlayRoomDialog::PlayerCount(PlayRoomTeam team) const {
    if (team >= PlayRoomTeam::Max) return 0;
    return m_teamPlayers[static_cast<std::size_t>(team)].size();
}

const std::string& cMNPlayRoomDialog::PlayerAt(PlayRoomTeam team, std::size_t i) const {
    if (team >= PlayRoomTeam::Max) {
        static const std::string empty;
        return empty;  // 1:1 quirk: out-of-range team returns empty.
    }
    auto& roster = m_teamPlayers[static_cast<std::size_t>(team)];
    if (i >= roster.size()) {
        static const std::string empty;
        return empty;  // 1:1 quirk: out-of-range index returns empty.
    }
    return roster[i];
}

} // namespace mxh::ui

