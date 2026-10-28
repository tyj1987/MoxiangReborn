// mnchanneldialog.hpp — modern port of 墨香 CMNChannelDialog
//
// 1:1 port of legacy `CMNChannelDialog` from
//   `墨香【源码】\[Client]MH\MNChannelDialog.{h,cpp}`.
//
// MurimNet (PvP server) channel selection dialog. The player can switch
// between 3 modes (ID list / channel list / playroom list) via 3
// cPushupButton tabs; each mode has its own cListDialog pane. There is
// also a chat cListDialog + cEditBox for in-channel chat.
//
// Children (1:1 with legacy MNCNL_*):
//   - 3 cListDialog (id list / channel list / playroom list)
//   - 3 cPushupButton (mode tab buttons)
//   - 1 cButton (join)
//   - 1 cEditBox (chat input)
//   - 1 cListDialog (chat log)
//   - 1 cStatic (title — resolved at SetChannelInfo time)
//
// Render is a no-op (resource-driven layout); the 6 add/remove/set
// methods are 1:1 wrappers around the child list-dialogs, with the
// mode-selection toggle implemented in SetChannelMode.

#pragma once

#include "cDialog.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace mxh::ui {

class cStatic;
class cButton;
class cEditBox;
class cListDialog;
class cPushupButton;

// 1:1 with legacy eCHANNEL_MODE enum in `[CC]Header/CommonGameDefine.h`.
// We inline the constants here (per AGENTS.md "1:1 contract preserved"
// rule — shared header must not be modified).
enum class ChannelMode : std::int32_t {
    Id       = 0,  // eCNL_MODE_ID
    Channel  = 1,  // eCNL_MODE_CHANNEL
    PlayRoom = 2,  // eCNL_MODE_PLAYROOM
    Max      = 3,  // eCNL_MODE_MAX
};

class cMNChannelDialog : public cDialog {
public:
    static constexpr int kNumModes = 3;  // 1:1 with eCNL_MODE_MAX

    // Local id range (1:1 with legacy MNCNL_* window ids, rebased to
    // 760..769 to avoid conflicts with previously-ported dialogs).
    static constexpr int kIdListDlgId       = 760;
    static constexpr int kIdListDlgChannel  = 761;
    static constexpr int kIdListDlgPlayRoom = 762;
    static constexpr int kIdBtnTabId        = 763;
    static constexpr int kIdBtnTabChannel   = 764;
    static constexpr int kIdBtnTabPlayRoom  = 765;
    static constexpr int kIdBtnJoin         = 766;
    static constexpr int kIdEdtChat         = 767;
    static constexpr int kIdListChat        = 768;
    static constexpr int kIdStcTitle        = 769;

    cMNChannelDialog();
    ~cMNChannelDialog() override;

    void Linking();
    void SetChannelMode(ChannelMode mode);
    void SetChannelInfo(const std::string& title);

    // 1:1 with legacy add/remove helpers (all 1:1 with the cpp body).
    void AddPlayer(const std::string& playerName, int level);
    void RemovePlayer(const std::string& playerName, int level);
    void RemoveAllPlayer();

    void AddChannel(const std::string& channelTitle,
                    std::uint16_t playerNum, std::uint16_t maxPlayer);
    void RemoveChannel(const std::string& channelTitle);
    void RemoveAllChannel();

    void AddPlayRoom(const std::string& playRoomTitle);
    void RemovePlayRoom(const std::string& playRoomTitle);
    void RemoveAllPlayRoom();

    // 1:1 with legacy ChatMsg(PRCTC_WHOLE) — formats "[name]: msg" and
    // pushes onto the chat list. Other nClass values (PRCTC_TEAM / etc.)
    // are not ported (no consumer in legacy body).
    void ChatMsgWhole(const std::string& playerName, const std::string& msg);

    // Test accessors.
    ChannelMode GetChannelMode() const noexcept { return m_channelMode; }
    const cListDialog* GetListDialogForMode(ChannelMode m) const noexcept;
    const cPushupButton* GetTabButtonForMode(ChannelMode m) const noexcept;
    const cStatic* GetTitle() const noexcept { return m_pTitle.get(); }
    const cButton* GetJoinButton() const noexcept { return m_pBtnJoin.get(); }
    const cEditBox* GetChatEdit() const noexcept { return m_pEdtChat.get(); }
    const cListDialog* GetChatList() const noexcept { return m_pLstChat.get(); }

private:
    // 1:1 quirk: legacy cListDialog* m_pListDlg[3] (raw) — modern
    // unique_ptr-as-member so tests can inspect without ownership aliasing.
    std::array<std::unique_ptr<cListDialog>,    kNumModes> m_pListDlg{};
    std::array<std::unique_ptr<cPushupButton>,  kNumModes> m_pBtnList{};

    std::unique_ptr<cButton>   m_pBtnJoin;
    std::unique_ptr<cEditBox>  m_pEdtChat;
    std::unique_ptr<cListDialog> m_pLstChat;
    std::unique_ptr<cStatic>   m_pTitle;

    ChannelMode m_channelMode = ChannelMode::Id;
};

} // namespace mxh::ui
