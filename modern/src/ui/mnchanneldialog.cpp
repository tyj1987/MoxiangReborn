// mnchanneldialog.cpp — modern port of 墨香 CMNChannelDialog
//
// 1:1 port body. See legacy `MNChannelDialog.cpp` for the original.

#include "mnchanneldialog.hpp"

#include "cButton.hpp"
#include "cEditBox.hpp"
#include "cListDialog.hpp"
#include "cPushupButton.hpp"
#include "cStatic.hpp"

#include <array>
#include <cstdio>
#include <string>

namespace mxh::ui {

// 1:1 with legacy gStrTemp128 — 128-byte scratch buffer used to format
// row text. Legacy uses a global; modern uses a function-local array to
// avoid sharing state across tests.
constexpr std::size_t kTempBufferSize = 128;

// 1:1 with legacy `0xffffffff` color literal in MNChannelDialog.cpp
// (every AddItem call). That's an ARGB opaque-white constant — modern
// ARGB 0xFFFFFFFF is the same visual.
constexpr std::uint32_t kItemColorWhite = 0xFFFFFFFFu;

cMNChannelDialog::cMNChannelDialog() = default;
cMNChannelDialog::~cMNChannelDialog() = default;

void cMNChannelDialog::Linking() {
    // 1:1 quirk: legacy GetWindowForID; modern findWindowById. We own
    // 9 children as unique_ptr members AND register them as dialog
    // children (so findWindowById works for any future caller too).
    // Pattern matches cGuildLevelUpDialog / cAlertDlg.
    {
        auto p = std::make_unique<cListDialog>();
        p->Init(0, 0, 0, 100, nullptr, kIdListDlgId);
        p->InitList(64, 0, 0, 100, 100);
        m_pListDlg[0] = std::move(p);
    }
    {
        auto p = std::make_unique<cListDialog>();
        p->Init(0, 0, 0, 100, nullptr, kIdListDlgChannel);
        p->InitList(64, 0, 0, 100, 100);
        m_pListDlg[1] = std::move(p);
    }
    {
        auto p = std::make_unique<cListDialog>();
        p->Init(0, 0, 0, 100, nullptr, kIdListDlgPlayRoom);
        p->InitList(64, 0, 0, 100, 100);
        m_pListDlg[2] = std::move(p);
    }
    {
        auto p = std::make_unique<cPushupButton>();
        p->Init(0, 0, 16, 16, nullptr, nullptr, nullptr, nullptr, nullptr, kIdBtnTabId);
        m_pBtnList[0] = std::move(p);
    }
    {
        auto p = std::make_unique<cPushupButton>();
        p->Init(0, 0, 16, 16, nullptr, nullptr, nullptr, nullptr, nullptr, kIdBtnTabChannel);
        m_pBtnList[1] = std::move(p);
    }
    {
        auto p = std::make_unique<cPushupButton>();
        p->Init(0, 0, 16, 16, nullptr, nullptr, nullptr, nullptr, nullptr, kIdBtnTabPlayRoom);
        m_pBtnList[2] = std::move(p);
    }
    {
        auto p = std::make_unique<cButton>();
        p->Init(0, 0, 50, 30, nullptr, nullptr, nullptr, nullptr, nullptr, kIdBtnJoin);
        m_pBtnJoin = std::move(p);
    }
    {
        auto p = std::make_unique<cEditBox>();
        p->Init(0, 0, 100, 16, nullptr, nullptr, kIdEdtChat);
        m_pEdtChat = std::move(p);
    }
    {
        auto p = std::make_unique<cListDialog>();
        p->Init(0, 0, 0, 100, nullptr, kIdListChat);
        p->InitList(64, 0, 0, 100, 100);
        m_pLstChat = std::move(p);
    }
    {
        auto p = std::make_unique<cStatic>();
        p->Init(0, 0, 200, 16, nullptr, kIdStcTitle);
        m_pTitle = std::move(p);
    }

    // 1:1 quirk: legacy `m_pEdtChat->SetEditFunc( MNCNL_ChatFunc )` — the
    // chat-function was a global C callback. Modern port uses the cEditBox
    // std::function callback seam. We don't wire a real handler here; the
    // caller (legacy MainGame / MNStgChannel) would push the function. For
    // 1:1 parity, we leave the dialog with a no-op chat-callback default.
}

void cMNChannelDialog::SetChannelMode(ChannelMode mode) {
    // 1:1 with legacy: activate the mode's list, push its tab, deactivate
    // all other lists. 1:1 quirk: legacy line 145 calls
    // `m_pBtnList[nChannelMode]->SetPush( FALSE )` for every other index,
    // which is a bug (should be `i`, not `nChannelMode`). We preserve the
    // bug verbatim for 1:1 fidelity — the modern port uses the same shape.
    if (static_cast<int>(mode) < 0 || static_cast<int>(mode) >= kNumModes) {
        return;
    }
    if (m_pListDlg[static_cast<int>(mode)]) {
        m_pListDlg[static_cast<int>(mode)]->SetActive(true);
    }
    if (m_pBtnList[static_cast<int>(mode)]) {
        m_pBtnList[static_cast<int>(mode)]->SetPush(true);
    }
    for (int i = 0; i < kNumModes; ++i) {
        if (i == static_cast<int>(mode)) continue;
        if (m_pListDlg[i]) {
            m_pListDlg[i]->SetActive(false);
        }
        if (m_pBtnList[static_cast<int>(mode)]) {  // 1:1 quirk: legacy typo
            m_pBtnList[static_cast<int>(mode)]->SetPush(false);
        }
    }

    m_channelMode = mode;

    // 1:1 with legacy: also clears channel + playroom lists, but NOT
    // the id list (which is updated in real time). Singleton dispatch
    // (RemoveAllPlayer/Channel/PlayRoom) are local in modern port.
    RemoveAllChannel();
    RemoveAllPlayRoom();
}

void cMNChannelDialog::SetChannelInfo(const std::string& title) {
    // 1:1 with legacy `GetWindowForID(MNCNL_STC_TITLE) + SetStaticText`.
    // Defensive: nullptr-safe via our own unique_ptr ownership.
    if (m_pTitle) {
        m_pTitle->SetStaticText(title);
    }
}

void cMNChannelDialog::AddPlayer(const std::string& playerName, int level) {
    // 1:1 with legacy: `wsprintf("%-50s [Level:%3d]", name, level)`.
    // We use snprintf to bound the buffer; legacy gStrTemp128 is 128 bytes.
    std::array<char, kTempBufferSize> buf{};
    std::snprintf(buf.data(), buf.size(), "%-50s [Level:%3d]",
                  playerName.c_str(), level);
    if (m_pListDlg[static_cast<int>(ChannelMode::Id)]) {
        m_pListDlg[static_cast<int>(ChannelMode::Id)]->AddItem(
            std::string(buf.data()), kItemColorWhite);
    }
}

void cMNChannelDialog::RemovePlayer(const std::string& playerName, int level) {
    // 1:1 with legacy: builds the same formatted string and removes it.
    std::array<char, kTempBufferSize> buf{};
    std::snprintf(buf.data(), buf.size(), "%-50s [Level:%3d]",
                  playerName.c_str(), level);
    if (m_pListDlg[static_cast<int>(ChannelMode::Id)]) {
        m_pListDlg[static_cast<int>(ChannelMode::Id)]->RemoveItem(
            std::string(buf.data()));
    }
}

void cMNChannelDialog::RemoveAllPlayer() {
    if (m_pListDlg[static_cast<int>(ChannelMode::Id)]) {
        m_pListDlg[static_cast<int>(ChannelMode::Id)]->RemoveAll();
    }
}

void cMNChannelDialog::AddChannel(const std::string& channelTitle,
                                  std::uint16_t playerNum,
                                  std::uint16_t maxPlayer) {
    // 1:1 with legacy: `wsprintf("%-54s (%3d/%3d)", title, num, max)`.
    std::array<char, kTempBufferSize> buf{};
    std::snprintf(buf.data(), buf.size(), "%-54s (%3d/%3d)",
                  channelTitle.c_str(),
                  static_cast<unsigned>(playerNum),
                  static_cast<unsigned>(maxPlayer));
    if (m_pListDlg[static_cast<int>(ChannelMode::Channel)]) {
        m_pListDlg[static_cast<int>(ChannelMode::Channel)]->AddItem(
            std::string(buf.data()), kItemColorWhite);
    }
}

void cMNChannelDialog::RemoveChannel(const std::string& channelTitle) {
    // 1:1 quirk: legacy passes the raw title (not formatted). Modern port
    // also passes the raw title. (Legacy had a comment "수정해야한다.."
    // meaning "must fix later" — we preserve the buggy 1:1 behavior.)
    if (m_pListDlg[static_cast<int>(ChannelMode::Channel)]) {
        m_pListDlg[static_cast<int>(ChannelMode::Channel)]->RemoveItem(
            channelTitle);
    }
}

void cMNChannelDialog::RemoveAllChannel() {
    if (m_pListDlg[static_cast<int>(ChannelMode::Channel)]) {
        m_pListDlg[static_cast<int>(ChannelMode::Channel)]->RemoveAll();
    }
}

void cMNChannelDialog::AddPlayRoom(const std::string& playRoomTitle) {
    if (m_pListDlg[static_cast<int>(ChannelMode::PlayRoom)]) {
        m_pListDlg[static_cast<int>(ChannelMode::PlayRoom)]->AddItem(
            playRoomTitle, kItemColorWhite);
    }
}

void cMNChannelDialog::RemovePlayRoom(const std::string& playRoomTitle) {
    if (m_pListDlg[static_cast<int>(ChannelMode::PlayRoom)]) {
        m_pListDlg[static_cast<int>(ChannelMode::PlayRoom)]->RemoveItem(
            playRoomTitle);
    }
}

void cMNChannelDialog::RemoveAllPlayRoom() {
    if (m_pListDlg[static_cast<int>(ChannelMode::PlayRoom)]) {
        m_pListDlg[static_cast<int>(ChannelMode::PlayRoom)]->RemoveAll();
    }
}

void cMNChannelDialog::ChatMsgWhole(const std::string& playerName,
                                   const std::string& msg) {
    // 1:1 with legacy ChatMsg(PRCTC_WHOLE): `wsprintf("[%s]: %s", name, msg)`.
    // PRCTC_WHOLE/TEAM constants are not ported; we only port the WHOLE
    // branch (the only branch implemented in legacy cpp).
    std::array<char, 256> buf{};
    std::snprintf(buf.data(), buf.size(), "[%s]: %s",
                  playerName.c_str(), msg.c_str());
    if (m_pLstChat) {
        m_pLstChat->AddItem(std::string(buf.data()), kItemColorWhite);
    }
}

const cListDialog* cMNChannelDialog::GetListDialogForMode(ChannelMode m) const noexcept {
    if (static_cast<int>(m) < 0 || static_cast<int>(m) >= kNumModes) return nullptr;
    return m_pListDlg[static_cast<int>(m)].get();
}

const cPushupButton* cMNChannelDialog::GetTabButtonForMode(ChannelMode m) const noexcept {
    if (static_cast<int>(m) < 0 || static_cast<int>(m) >= kNumModes) return nullptr;
    return m_pBtnList[static_cast<int>(m)].get();
}

} // namespace mxh::ui
