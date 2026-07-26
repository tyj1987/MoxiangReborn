// cchanneldialog.hpp — modern port of 墨香 CChannelDialog.
//
// 1:1 port of legacy `CChannelDialog` from
//   `墨香【源码】\[Client]MH\ChannelDialog.{h,cpp}`.
//
// The channel dialog is a server-side channel picker shown after
// character selection.  The legacy:
//   * SetChannelList(MSG_CHANNEL_INFO*) populates a cListCtrl
//     with one row per channel (channel name + crowd level +
//     battle-channel tag), picks the lowest-crowd row as the
//     default, then SetActive(TRUE).
//   * ActionEvent on WE_ROWCLICK calls SelectChannel(rowidx).
//   * WE_ROWDBLCLICK calls OnConnect() (or MapChange() in
//     _KOR_LOCAL_).
//   * OnConnect() checks gChannelNum != -1, then calls
//     CHARSELECT->EnterGame() (1:1 stubbed via a callback).
//   * SelectChannel(rowidx) updates the highlighted row + the
//     global gChannelNum (gated by the legacy's
//     "pRItem->dwID >= 300 -> gChannelNum = -1" rule).
//
// The modern port keeps the full 1:1 surface, with the
// cross-cutting global state (gChannelNum, MAINGAME, CHARSELECT,
// HERO, NETWORK, CHATMGR) replaced by host-injected callbacks.
// The 4 callbacks the host wires are:
//   * GetCrowdLevelString(playerNum) -> chatmsg-indexed string
//   * GetBattleChannelTag()          -> chatmsg-indexed string
//   * DisplayNotice(chatmsgId)        -> show a notice
//   * EnterGame()                    -> start the char-select flow
//
// The MSG_CHANNEL_INFO struct is also ported 1:1 so the channel
// list payload is binary-compatible (the host can re-use the
// network msg buffer).

#pragma once

#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cwindow.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mxh::ui {

class cListCtrl;

// 1:1 with legacy MSG_CHANNEL_INFO (legacy ChannelDialog.cpp).
// The struct preserves the legacy array sizes so the network
// message remains binary-compatible.  Field order matches the
// legacy C struct; do not reorder.
struct MSG_CHANNEL_INFO {
    char  ChannelName[64] = {};
    std::uint16_t Count = 0;
    std::uint16_t PlayerNum[100] = {};
    bool bBattleChannel[100] = {};
    std::uint16_t wMoveMapNum = 0;        // _KOR_LOCAL_ field, kept always
    std::uint32_t dwChangeMapState = 0;   // _KOR_LOCAL_ field, kept always
};

// 1:1 with legacy MAX_CHANNEL_NUM = 100 (used by the global
// IsBattleChannel array in ChannelDialog.cpp).
inline constexpr std::int32_t kMaxChannelNum = 100;

// 1:1 with legacy MAX_CHANNEL_NAME+4 = 68.
inline constexpr std::int32_t kMaxChannelNameBuf = 64 + 4;

class cChannelDialog : public cDialog {
public:
    cChannelDialog();
    ~cChannelDialog() override;

    cChannelDialog(const cChannelDialog&) = delete;
    cChannelDialog& operator=(const cChannelDialog&) = delete;

    // 1:1 with legacy SetChannelList(MSG_CHANNEL_INFO*).  Builds
    // the channel list + picks the lowest-crowd row as default +
    // SetActive(true).
    void SetChannelList(const MSG_CHANNEL_INFO& info);

    // 1:1 with legacy SetActive(BOOL).
    void SetActive(bool val) noexcept override;

    // 1:1 with legacy ActionEvent.  WE_ROWCLICK -> SelectChannel;
    // WE_ROWDBLCLICK -> OnConnect.  The host flags the WE
    // bits via SetLastActionEventWeForTest (modern cMouse
    // port is deferred).
    std::uint32_t ActionEvent(/*CMouse**/ void* mouseInfo);

    // Test hook -- set the WE bits the next ActionEvent
    // call dispatches.  The legacy reads the WE bits out of
    // the CMouse; the modern port lets tests inject them
    // directly so unit tests don't need a full CMouse.
    void SetLastActionEventWeForTest(std::uint32_t we) noexcept { m_lastWe = we; }

    // 1:1 with legacy Linking.  Wires m_pChannelLCtrl.
    void Linking();

    // 1:1 with legacy SelectChannel(int).
    void SelectChannel(int rowidx);

    // 1:1 with legacy OnConnect().  Calls the host's
    // DisplayNotice or EnterGame callback based on gChannelNum.
    void OnConnect();

    // 1:1 with legacy SendMapChannelInfoSYN.  KOR_LOCAL_ only;
    // modern port keeps the API surface and lets the host
    // decide what to do with the (wMapNum, dwState) pair.
    using SendMapChannelInfoCallback = void(*)(std::uint16_t wMapNum,
                                                std::uint32_t dwState,
                                                void* user);
    void SetSendMapChannelInfoCallbackForTest(SendMapChannelInfoCallback cb,
                                                void* user) {
        m_sendMapCb = cb; m_sendMapUser = user;
    }
    void SendMapChannelInfoSYN(std::uint16_t wMapNum,
                                std::uint32_t dwState = 0 /* eMapChange_General */);

    // 1:1 with legacy MapChange.  KOR_LOCAL_ only.
    using MapChangeCallback = void(*)(std::uint16_t wMapNum, void* user);
    using AddSysMsgCallback = void(*)(int chatMsgId, void* user);
    void SetMapChangeCallbackForTest(MapChangeCallback cb, void* user) {
        m_mapChangeCb = cb; m_mapChangeUser = user;
    }
    void SetAddSysMsgCallbackForTest(AddSysMsgCallback cb, void* user) {
        m_addSysMsgCb = cb; m_addSysMsgUser = user;
    }
    void MapChange();

    // Test hook -- "is the dialog ready" flag (1:1 with m_bInit).
    bool isInit() const noexcept { return m_bInit; }

    // Test introspection.
    int               baseChannelIndex() const noexcept { return m_BaseChannelIndex; }
    int               selectRowIdx()     const noexcept { return m_SelectRowIdx; }
    int&              selectRowIdxForTest()      noexcept { return m_SelectRowIdx; }
    std::uint16_t     moveMapNum()       const noexcept { return m_wMoveMapNum; }
    std::uint32_t     changeMapState()   const noexcept { return m_dwChangeMapState; }
    cListCtrl*        channelList()      const noexcept { return m_pChannelLCtrl; }

    // 1:1 with legacy global gChannelNum.  Test hook exposes
    // the read/write so tests can drive OnConnect paths.
    static int  GlobalChannelNum() noexcept;
    static void SetGlobalChannelNumForTest(int v) noexcept;

    // 1:1 with legacy global IsBattleChannel[].
    static const bool* GlobalIsBattleChannel() noexcept;
    static void        SetGlobalIsBattleChannelForTest(int idx, bool v) noexcept;

    // 1:1 with legacy CHATMGR chatmsg lookup (cChatManager::
    // GetChatMsg).  Legacy: returns the localised string for
    // a chatmsg id.  Modern port: host injects the lookup.
    using ChatMsgCallback = const char*(*)(int chatMsgId, void* user);
    void SetChatMsgCallbackForTest(ChatMsgCallback cb, void* user) {
        m_chatMsgCb = cb; m_chatMsgUser = user;
    }

    // 1:1 with legacy CHARSELECT->DisplayNotice / EnterGame.
    using DisplayNoticeCallback = void(*)(int chatMsgId, void* user);
    using EnterGameCallback     = bool(*)(void* user);
    void SetDisplayNoticeCallbackForTest(DisplayNoticeCallback cb, void* user) {
        m_displayNoticeCb = cb; m_displayNoticeUser = user;
    }
    void SetEnterGameCallbackForTest(EnterGameCallback cb, void* user) {
        m_enterGameCb = cb; m_enterGameUser = user;
    }

    // 1:1 with legacy chatmsg ids used by ChannelDialog.cpp.
    static constexpr int kChatMsgDisplayNotice_279 = 279;
    static constexpr int kChatMsgDisplayNotice_18  = 18;
    static constexpr int kChatMsgCrowdLow_211      = 211;
    static constexpr int kChatMsgCrowdMid_212      = 212;
    static constexpr int kChatMsgCrowdHigh_213     = 213;
    static constexpr int kChatMsgChannel_1701      = 1701;
    static constexpr int kChatMsgBattleTag_1702    = 1702;
    static constexpr int kChatMsgMapChangeFailed_1699 = 1699;

    // 1:1 with legacy crowd thresholds (the CN/KOR default
    // branch is < 40 / < 100 / >= 100).
    static constexpr std::int32_t kCrowdLowThreshold  = 40;
    static constexpr std::int32_t kCrowdMidThreshold  = 100;
    static constexpr std::int32_t kCrowdFullThreshold = 300;

    // 1:1 with legacy RGBA_MAKE(255,234,0,255) for the
    // highlighted row.
    static constexpr std::uint32_t kHighlightRgba = 0xFFFFEA00u;
    static constexpr std::uint32_t kDefaultRgba    = 0xFFFFFFFFu;
    static constexpr std::uint32_t kRedRgba        = 0xFF0000FFu;

    // Per-channel row data.  Mirrors the legacy cRITEMEx
    // (2 columns: name, crowd).  Tests can introspect via
    // rowCountForTest / rowNameForTest / rowPlayerNumForTest.
    struct Row {
        std::string name;
        std::string crowd;
        std::uint16_t playerNum = 0;
        bool          battle = false;
    };

    int             rowCountForTest() const noexcept { return static_cast<int>(m_rows.size()); }
    const Row&      rowForTest(int idx) const { return m_rows.at(static_cast<std::size_t>(idx)); }
    int             channelCount()     const noexcept { return static_cast<int>(m_rows.size()); }

    // 1:1 with legacy SetActive: also clears the global
    // IsBattleChannel array on close.
    void SetActiveExt(bool val);

private:
    void ClearRows();
    void FormatChannelName(char* dst, std::size_t dstSize, int idx) const;
    void FormatCrowdLevel(char* dst, std::size_t dstSize, int playerNum) const;
    void AppendBattleTag(char* dst, std::size_t dstSize) const;

    cListCtrl* m_pChannelLCtrl = nullptr;
    int        m_BaseChannelIndex = 0;
    int        m_SelectRowIdx = 0;
    bool       m_bInit = false;
    std::uint16_t m_wMoveMapNum = 0;
    std::uint32_t m_dwChangeMapState = 0;

    std::vector<Row> m_rows;

    ChatMsgCallback       m_chatMsgCb = nullptr;
    void*                 m_chatMsgUser = nullptr;
    DisplayNoticeCallback m_displayNoticeCb = nullptr;
    void*                 m_displayNoticeUser = nullptr;
    EnterGameCallback     m_enterGameCb = nullptr;
    void*                 m_enterGameUser = nullptr;
    SendMapChannelInfoCallback m_sendMapCb = nullptr;
    void*                     m_sendMapUser = nullptr;
    MapChangeCallback      m_mapChangeCb = nullptr;
    void*                  m_mapChangeUser = nullptr;
    AddSysMsgCallback      m_addSysMsgCb = nullptr;
    void*                  m_addSysMsgUser = nullptr;
    std::uint32_t          m_lastWe = 0;
};

} // namespace mxh::ui
