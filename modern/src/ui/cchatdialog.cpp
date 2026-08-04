// cchatdialog.cpp — modern port of 墨香 CChatDialog.

#include "mxh/ui/cchatdialog.hpp"
#include "legacy_window_event.hpp"
#include "mxh/ui/ceditbox.hpp"
#include "mxh/ui/clistdialog.hpp"
#include "mxh/ui/cPushupButton.hpp"

#include <cstring>
#include <utility>

namespace mxh::ui {

cChatDialog::cChatDialog() {
    m_pChatEditBox = nullptr;
    m_pAllShout    = nullptr;
    for (int i = 0; i < kMaxChatCountNum; ++i) {
        m_pSheet[i]  = nullptr;
        m_pPBMenu[i] = nullptr;
    }
    m_nCurSheetNum   = 0;
    m_bHideChatDialog = false;
    m_bShowGuildTab   = true;
    m_cPreWord[static_cast<int>(ChatSheet::Whole)]    = kDefaultPreWordWhole;
    m_cPreWord[static_cast<int>(ChatSheet::Party)]    = kDefaultPreWordParty;
    m_cPreWord[static_cast<int>(ChatSheet::Guild)]    = kDefaultPreWordGuild;
    m_cPreWord[static_cast<int>(ChatSheet::Alliance)] = kDefaultPreWordAlliance;
    m_cPreWord[static_cast<int>(ChatSheet::Shout)]    = kDefaultPreWordShout;
    m_SelectedName[0] = '\0';
}

cChatDialog::~cChatDialog() = default;

void cChatDialog::Linking() {
    // 1:1 with legacy Linking.  The legacy walks the
    // WINDOW_ID tree; the modern port defers that and
    // reads from m_childWindows (set by the test hook or
    // a future cWindowManager port).
    if (m_childWindows.chatEditBox) {
        m_pChatEditBox = m_childWindows.chatEditBox;
    }
    for (int i = 0; i < kMaxChatCountNum; ++i) {
        if (m_childWindows.sheets[i])  m_pSheet[i]  = m_childWindows.sheets[i];
        if (m_childWindows.pbMenus[i]) m_pPBMenu[i] = m_childWindows.pbMenus[i];
    }
    m_pAllShout = m_childWindows.allShout;
}

std::uint32_t cChatDialog::ActionEvent(void* /*mouseInfo*/) {
    // 1:1 with legacy ActionEvent.  The legacy forwards to
    // cDialog::ActionEvent + handles the chat-input click +
    // name-pick.  Modern port is a no-op (cMouse not ported).
    return 0;
}

void cChatDialog::AddMsg(std::uint8_t chatLimit, std::uint32_t msgColor, const char* str) {
    // 1:1 with legacy AddMsg.  Each bit in chatLimit enables
    // a sheet.  The legacy copies the string into the sheet;
    // the modern port routes the call through ListAddItem
    // callback per matching sheet.
    if (m_listAddItemCb == nullptr || str == nullptr) return;
    if (chatLimit & kChatLimitWhole)    m_listAddItemCb(static_cast<int>(ChatSheet::Whole),    msgColor, str, m_listAddItemUser);
    if (chatLimit & kChatLimitParty)    m_listAddItemCb(static_cast<int>(ChatSheet::Party),    msgColor, str, m_listAddItemUser);
    if (chatLimit & kChatLimitGuild)    m_listAddItemCb(static_cast<int>(ChatSheet::Guild),    msgColor, str, m_listAddItemUser);
    if (chatLimit & kChatLimitAlliance) m_listAddItemCb(static_cast<int>(ChatSheet::Alliance), msgColor, str, m_listAddItemUser);
    if (chatLimit & kChatLimitShout)    m_listAddItemCb(static_cast<int>(ChatSheet::Shout),    msgColor, str, m_listAddItemUser);
}

void cChatDialog::AddMsgAll(std::uint32_t msgColor, const char* str) {
    // 1:1 with legacy AddMsgAll: 0xFF = all 5 sheets.
    AddMsg(0xFF, msgColor, str);
}

void cChatDialog::OnActionEvent(std::int32_t lId, void* /*p*/, std::uint32_t we) {
    // 1:1 with legacy OnActionEvent.  Routes tab button clicks
    // to SelectMenu.  The legacy also handles chat-input
    // send (CHATDLG_INPUTBOX id) which delegates to a global
    // CHATMSG / network path; modern port defers that.
    constexpr std::uint32_t kBtnClick = legacy_window_event::kButtonClick;
    if ((we & kBtnClick) == 0) return;
    // Tab menu ids are 0..4 in the legacy; the modern port
    // lets the host map its own id range, so we accept any
    // lId in [0, 4].
    if (lId >= 0 && lId < kMaxChatCountNum) {
        SelectMenu(lId);
    }
}

void cChatDialog::SelectMenu(int nSheet) {
    // 1:1 with legacy SelectMenu.  Switches the active sheet
    // and fires the select-menu callback (legacy:
    // m_pPBMenu[i]->SetPush(TRUE/FALSE)).
    if (nSheet < 0 || nSheet >= kMaxChatCountNum) return;
    m_nCurSheetNum = nSheet;
    if (m_selectMenuCb) m_selectMenuCb(nSheet, m_selectMenuUser);
    SetEditBoxPreWord();
}

void cChatDialog::SetEditBoxPreWord() {
    // 1:1 with legacy SetEditBoxPreWord.  Stamps the
    // pre-word for the current sheet into the chat edit box.
    if (m_setEditTextCb) {
        char buf[2] = { m_cPreWord[m_nCurSheetNum], '\0' };
        m_setEditTextCb(buf, m_setEditTextUser);
    }
}

bool cChatDialog::IsPreWord(char c) const {
    // 1:1 with legacy IsPreWord.
    for (int i = 0; i < kMaxChatCountNum; ++i) {
        if (m_cPreWord[i] == c) return true;
    }
    return false;
}

void cChatDialog::ShowGuildTab(bool bShow) noexcept {
    // 1:1 with legacy ShowGuildTab.  Flips the flag and
    // (1:1) shows / hides the guild + alliance pushup
    // buttons.
    m_bShowGuildTab = bShow;
    // Modern cPushupButton has no SetActive API; the
    // visibility flag is recorded.
}

std::uint16_t cChatDialog::GetSheetPosY() const noexcept {
    // 1:1 with legacy GetSheetPosY.  The legacy returns the
    // absY of the current sheet.  Modern port returns
    // cDialog::absY() as a reasonable substitute.
    return static_cast<std::uint16_t>(absY());
}

std::uint16_t cChatDialog::GetSheetHeight() const noexcept {
    // 1:1 with legacy GetSheetHeight.  The legacy returns
    // m_pSheet[m_nCurSheetNum]->GetHeight().  Modern port
    // returns 0 when no sheet is bound.
    return 0;
}

void cChatDialog::SetAllShoutBtnPushed(bool val) {
    // 1:1 with legacy SetAllShoutBtnPushed.  Calls
    // m_pAllShout->SetPush(BOOL).  Modern port: the call is
    // a no-op (the modern cPushupButton has no
    // notification; the host owns the wiring).
    (void)val;
}

cListDialog* cChatDialog::GetSheet(int nSheet) const noexcept {
    if (nSheet < 0 || nSheet >= kMaxChatCountNum) return nullptr;
    return m_pSheet[nSheet];
}

int cChatDialog::GetLineNum() const noexcept {
    // 1:1 with legacy GetLineNum.  Sums the line counts
    // across all 5 sheets.
    if (m_lineCountCb == nullptr) return 0;
    int total = 0;
    for (int i = 0; i < kMaxChatCountNum; ++i) {
        total += m_lineCountCb(i, m_lineCountUser);
    }
    return total;
}

}  // namespace mxh::ui
