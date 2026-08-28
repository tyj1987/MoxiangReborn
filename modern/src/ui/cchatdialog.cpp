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
    // Prefer explicit test wiring, then resolve the shipped CHATDLG tree by
    // its legacy IDs. This is the same control topology used by the client
    // runtime, not a callback-only test substitute.
    if (!m_childWindows.chatEditBox) {
        m_childWindows.chatEditBox = dynamic_cast<cEditBox*>(
            findWindowByLegacyId("MI_CHATEDITBOX"));
    }
    static constexpr const char* kSheetIds[kMaxChatCountNum]{
        "CTI_SHEET1", "CTI_SHEET2", "CTI_SHEET3", "CTI_SHEET4", "CTI_SHEET5"};
    static constexpr const char* kMenuIds[kMaxChatCountNum]{
        "CTI_BTN_WHOLE", "CTI_BTN_PARTY", "CTI_BTN_MUNPA",
        "CTI_BTN_ALLMUNPA", "CTI_BTN_WORLD"};
    if (!m_childWindows.allShout) {
        m_childWindows.allShout = dynamic_cast<cPushupButton*>(
            findWindowByLegacyId("CTI_BTN_ALLWORLD1"));
    }
    for (int i = 0; i < kMaxChatCountNum; ++i) {
        if (!m_childWindows.sheets[i]) {
            m_childWindows.sheets[i] = dynamic_cast<cListDialog*>(
                findWindowByLegacyId(kSheetIds[i]));
        }
        if (!m_childWindows.pbMenus[i]) {
            m_childWindows.pbMenus[i] = dynamic_cast<cPushupButton*>(
                findWindowByLegacyId(kMenuIds[i]));
        }
    }
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
    if (str == nullptr) return;
    const auto add = [this, msgColor, str](int sheet) {
        if (sheet >= 0 && sheet < kMaxChatCountNum && m_pSheet[sheet]) {
            m_pSheet[sheet]->AddItem(str, msgColor);
        }
        if (m_listAddItemCb) {
            m_listAddItemCb(sheet, msgColor, str, m_listAddItemUser);
        }
    };
    if (chatLimit & kChatLimitWhole)    add(static_cast<int>(ChatSheet::Whole));
    if (chatLimit & kChatLimitParty)    add(static_cast<int>(ChatSheet::Party));
    if (chatLimit & kChatLimitGuild)    add(static_cast<int>(ChatSheet::Guild));
    if (chatLimit & kChatLimitAlliance) add(static_cast<int>(ChatSheet::Alliance));
    if (chatLimit & kChatLimitShout)    add(static_cast<int>(ChatSheet::Shout));
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
    for (int i = 0; i < kMaxChatCountNum; ++i) {
        if (m_pPBMenu[i]) m_pPBMenu[i]->SetPush(i == nSheet);
    }
    if (m_selectMenuCb) m_selectMenuCb(nSheet, m_selectMenuUser);
    SetEditBoxPreWord();
}

void cChatDialog::SetEditBoxPreWord() {
    // 1:1 with legacy SetEditBoxPreWord.  Stamps the
    // pre-word for the current sheet into the chat edit box.
    char buf[2] = { m_cPreWord[m_nCurSheetNum], '\0' };
    if (m_pChatEditBox) m_pChatEditBox->SetEditText(buf);
    if (m_setEditTextCb) m_setEditTextCb(buf, m_setEditTextUser);
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
    if (m_pPBMenu[static_cast<int>(ChatSheet::Guild)])
        m_pPBMenu[static_cast<int>(ChatSheet::Guild)]->SetActive(bShow);
    if (m_pPBMenu[static_cast<int>(ChatSheet::Alliance)])
        m_pPBMenu[static_cast<int>(ChatSheet::Alliance)]->SetActive(bShow);
}

std::uint16_t cChatDialog::GetSheetPosY() const noexcept {
    // 1:1 with legacy GetSheetPosY.  The legacy returns the
    // absY of the current sheet.  Modern port returns
    // cDialog::absY() as a reasonable substitute.
    const auto* sheet = m_pSheet[m_nCurSheetNum];
    return static_cast<std::uint16_t>(sheet ? sheet->absY() : absY());
}

std::uint16_t cChatDialog::GetSheetHeight() const noexcept {
    // 1:1 with legacy GetSheetHeight.  The legacy returns
    // m_pSheet[m_nCurSheetNum]->GetHeight().  Modern port
    // returns 0 when no sheet is bound.
    const auto* sheet = m_pSheet[m_nCurSheetNum];
    return static_cast<std::uint16_t>(sheet ? sheet->height() : 0);
}

void cChatDialog::SetAllShoutBtnPushed(bool val) {
    // 1:1 with legacy SetAllShoutBtnPushed.  Calls
    // m_pAllShout->SetPush(BOOL).  Modern port: the call is
    // a no-op (the modern cPushupButton has no
    // notification; the host owns the wiring).
    if (m_pAllShout) m_pAllShout->SetPush(val);
}

cListDialog* cChatDialog::GetSheet(int nSheet) const noexcept {
    if (nSheet < 0 || nSheet >= kMaxChatCountNum) return nullptr;
    return m_pSheet[nSheet];
}

int cChatDialog::GetLineNum() const noexcept {
    // 1:1 with legacy GetLineNum.  Sums the line counts
    // across all 5 sheets.
    int total = 0;
    for (int i = 0; i < kMaxChatCountNum; ++i) {
        if (m_pSheet[i]) total += static_cast<int>(m_pSheet[i]->RowCount());
        else if (m_lineCountCb) total += m_lineCountCb(i, m_lineCountUser);
    }
    return total;
}

}  // namespace mxh::ui
