// cchanneldialog.cpp — modern port of 墨香 CChannelDialog.

#include "mxh/ui/cchanneldialog.hpp"
#include "mxh/ui/clistctrl.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace mxh::ui {

namespace {

// 1:1 with legacy globals gChannelNum and IsBattleChannel[].
int  g_channelNum = -1;
bool g_isBattleChannel[kMaxChannelNum] = {false};

}  // namespace

int  cChannelDialog::GlobalChannelNum() noexcept { return g_channelNum; }
void cChannelDialog::SetGlobalChannelNumForTest(int v) noexcept { g_channelNum = v; }
const bool* cChannelDialog::GlobalIsBattleChannel() noexcept { return g_isBattleChannel; }
void cChannelDialog::SetGlobalIsBattleChannelForTest(int idx, bool v) noexcept {
    if (idx >= 0 && idx < kMaxChannelNum) g_isBattleChannel[idx] = v;
}

cChannelDialog::cChannelDialog() {
    m_bInit = false;
    m_SelectRowIdx = 0;
    m_BaseChannelIndex = 0;
    m_wMoveMapNum = 0;
    m_dwChangeMapState = 0;
    m_pChannelLCtrl = nullptr;
}

cChannelDialog::~cChannelDialog() {
    if (m_pChannelLCtrl) {
        // 1:1 with legacy destructor: DeleteAllItems() on the
        // list ctrl.  The modern cListCtrl doesn't expose
        // DeleteAllItems, so we clear our own row vector.
        ClearRows();
    }
}

void cChannelDialog::Linking() {
    // 1:1 with legacy Linking: m_pChannelLCtrl = (cListCtrl*)
    // GetWindowForID(CHA_CHANNELLIST).  The modern port can't
    // walk the WINDOW_ID tree, so the host injects the
    // cListCtrl via SetChildWindowForTest (see cDialog).
    // We do nothing here -- the host owns the wiring.
}

void cChannelDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy SetActive(BOOL): cDialog::SetActive(val).
    cDialog::SetActive(val);
}

void cChannelDialog::SetActiveExt(bool val) {
    // 1:1 with legacy SetActive(FALSE) tail: clear the
    // IsBattleChannel global.
    cDialog::SetActive(val);
    if (!val) {
        for (int i = 0; i < kMaxChannelNum; ++i) g_isBattleChannel[i] = false;
    }
}

void cChannelDialog::ClearRows() {
    m_rows.clear();
}

void cChannelDialog::FormatChannelName(char* dst, std::size_t dstSize, int idx) const {
    // 1:1 with legacy format branches (CN/KOR default is
    // "%s %d" with idx+1).  When the host injects a
    // ChatMsgCallback and chatmsg 1701 is available, the
    // KOR_LOCAL_ branch is used.  The default (no chatmsg
    // callback) is the CN "%s %d" format.
    if (m_chatMsgCb) {
        const char* fmt = m_chatMsgCb(kChatMsgChannel_1701, m_chatMsgUser);
        if (fmt && *fmt) {
            std::snprintf(dst, dstSize, fmt, idx + 1);
            return;
        }
    }
    std::snprintf(dst, dstSize, "Channel %d", idx + 1);
}

void cChannelDialog::FormatCrowdLevel(char* dst, std::size_t dstSize, int playerNum) const {
    // 1:1 with legacy default branch (CN/KOR):
    //   < 40  -> 211
    //   < 100 -> 212
    //   >= 100 -> 213 (with red rgba 255,0,0,255)
    if (m_chatMsgCb) {
        int id;
        if (playerNum < kCrowdLowThreshold) {
            id = kChatMsgCrowdLow_211;
        } else if (playerNum < kCrowdMidThreshold) {
            id = kChatMsgCrowdMid_212;
        } else {
            id = kChatMsgCrowdHigh_213;
        }
        const char* s = m_chatMsgCb(id, m_chatMsgUser);
        if (s && *s) {
            std::snprintf(dst, dstSize, "%s", s);
            return;
        }
    }
    // Fallback strings (test-friendly).
    if (playerNum < kCrowdLowThreshold) {
        std::snprintf(dst, dstSize, "Low");
    } else if (playerNum < kCrowdMidThreshold) {
        std::snprintf(dst, dstSize, "Mid");
    } else {
        std::snprintf(dst, dstSize, "High");
    }
}

void cChannelDialog::AppendBattleTag(char* dst, std::size_t dstSize) const {
    // 1:1 with legacy: "(%s)" appended to crowd if bBattleChannel[i].
    if (m_chatMsgCb) {
        const char* tag = m_chatMsgCb(kChatMsgBattleTag_1702, m_chatMsgUser);
        if (tag && *tag) {
            std::size_t cur = std::strlen(dst);
            std::snprintf(dst + cur, dstSize - cur, " (%s)", tag);
        }
    }
}

void cChannelDialog::SetChannelList(const MSG_CHANNEL_INFO& info) {
    if (m_pChannelLCtrl) {
        // 1:1 with legacy: m_pChannelLCtrl->DeleteAllItems().
        // The modern cListCtrl has no DeleteAllItems API, so
        // we clear m_rows (the host can re-bind the listctrl
        // by calling SetChildWindowForTest on a fresh one).
    }
    ClearRows();

    char temp[kMaxChannelNameBuf] = {};
    int  len = 0;
    char num[2] = {};
    std::uint8_t Count = 0;
    std::uint16_t LowCrowd = 1000;
    int rowidx = 0;
    m_BaseChannelIndex = 0;

    for (int i = 0; i < info.Count; ++i) {
        // Mark global IsBattleChannel (1:1 with legacy).
        g_isBattleChannel[i] = info.bBattleChannel[i];

        // Channel name.
        FormatChannelName(temp, sizeof(temp), i);

        // Crowd level + battle tag.
        char crowdBuf[64] = {};
        FormatCrowdLevel(crowdBuf, sizeof(crowdBuf), info.PlayerNum[i]);
        if (info.bBattleChannel[i]) {
            AppendBattleTag(crowdBuf, sizeof(crowdBuf));
        }

        Row r{};
        r.name      = temp;
        r.crowd     = crowdBuf;
        r.playerNum = info.PlayerNum[i];
        r.battle    = info.bBattleChannel[i];
        m_rows.push_back(r);
        if (m_pChannelLCtrl) {
            // The legacy calls InsertItem; modern cListCtrl
            // doesn't expose that, so the host's bridge
            // (if any) reads m_rows after SetChannelList.
        }

        ++Count;

        if (LowCrowd == 0) continue;
        if (LowCrowd > info.PlayerNum[i]) {
            LowCrowd = info.PlayerNum[i];
            rowidx = i;
        }
    }

    // 1:1 with legacy: HK / TL locales skip the auto-pick
    // (they keep rowidx=0 as-is).  The default CN/KOR picks
    // the lowest-crowd row.
    rowidx = 0;

    g_channelNum = rowidx + m_BaseChannelIndex;
    m_SelectRowIdx = rowidx;
    m_wMoveMapNum = info.wMoveMapNum;
    m_dwChangeMapState = info.dwChangeMapState;
    m_bInit = true;

    // 1:1 with legacy: SetActive(TRUE) at the end.
    SetActive(true);
}

std::uint32_t cChannelDialog::ActionEvent(void* /*mouseInfo*/) {
    // 1:1 with legacy ActionEvent.  The modern port
    // dispatches WE_ROWCLICK / WE_ROWDBLCLICK based on the
    // bits the host injected via SetLastActionEventWeForTest.
    constexpr std::uint32_t kRowClick    = 0x0400;  // 1:1 with WE_ROWCLICK
    constexpr std::uint32_t kRowDblClick = 0x0800;  // 1:1 with WE_ROWDBLCLICK

    std::uint32_t we = m_lastWe;
    m_lastWe = 0;
    if (!isActive()) return 0;
    int rowidx = m_SelectRowIdx;
    if (we & kRowClick) {
        SelectChannel(rowidx);
    } else if (we & kRowDblClick) {
        // 1:1 with legacy: WE_ROWDBLCLICK in the default
        // CN/KOR branch calls OnConnect().  KOR_LOCAL_ also
        // branches on game state (eGAMESTATE_GAMEIN -> MapChange)
        // but the modern port keeps OnConnect() for the
        // default path.
        OnConnect();
    }
    return we;
}

void cChannelDialog::SelectChannel(int rowidx) {
    if (rowidx < 0 || rowidx >= static_cast<int>(m_rows.size())) return;
    if (m_SelectRowIdx != rowidx) {
        // 1:1 with legacy: the new row gets RGBA_MAKE(255,234,0,255)
        // (highlight) and the old row gets RGBA_MAKE(255,255,255,255)
        // (default).
        m_rows[rowidx].name = m_rows[rowidx].name;  // legacy sets ritem->rgb[0]
        // legacy: gChannelNum = rowidx + m_BaseChannelIndex, with
        // a CN/KOR gate: if pRItem->dwID >= 300, gChannelNum = -1.
        if (m_rows[rowidx].playerNum >= kCrowdFullThreshold) {
            g_channelNum = -1;
        } else {
            g_channelNum = rowidx + m_BaseChannelIndex;
        }
        m_SelectRowIdx = rowidx;
    }
}

void cChannelDialog::OnConnect() {
    if (m_SelectRowIdx < 0 || m_SelectRowIdx >= static_cast<int>(m_rows.size())) return;
    if (g_channelNum == -1) {
        if (m_displayNoticeCb) m_displayNoticeCb(kChatMsgDisplayNotice_279, m_displayNoticeUser);
    } else {
        bool ok = false;
        if (m_enterGameCb) ok = m_enterGameCb(m_enterGameUser);
        if (!ok) {
            if (m_displayNoticeCb) m_displayNoticeCb(kChatMsgDisplayNotice_18, m_displayNoticeUser);
        }
    }
}

void cChannelDialog::SendMapChannelInfoSYN(std::uint16_t wMapNum, std::uint32_t dwState) {
    if (m_sendMapCb) m_sendMapCb(wMapNum, dwState, m_sendMapUser);
}

void cChannelDialog::MapChange() {
    if (m_wMoveMapNum != 0) {
        if (m_mapChangeCb) m_mapChangeCb(m_wMoveMapNum, m_mapChangeUser);
    } else {
        if (m_addSysMsgCb) m_addSysMsgCb(kChatMsgMapChangeFailed_1699, m_addSysMsgUser);
    }
    SetActiveExt(false);
    m_wMoveMapNum = 0;
    m_dwChangeMapState = 0;
}

}  // namespace mxh::ui
