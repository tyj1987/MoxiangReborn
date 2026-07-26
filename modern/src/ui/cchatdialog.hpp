// cchatdialog.hpp — modern port of 墨香 CChatDialog (chat UI with 5 sheets).
//
// 1:1 port of legacy `CChatDialog` from
//   `墨香【源码】\[Client]MH\ChatDialog.h` (no .cpp in legacy).
//
// The chat dialog has 5 sheets (WHOLE / PARTY / GUILD /
// ALLIANCE / SHOUT), each backed by a cListDialog + a
// cPushupButton tab.  AddMsg(ChatLimit, color, str) routes
// the message to the matching sheet(s).  AddMsgAll goes to
// every sheet.
//
// The modern port keeps the 1:1 surface with the cross-cutting
// CHATMSG / network dispatch stubbed via host-injected callbacks
// or test hooks.  Sheet writes are routed through a callback
// the host injects (the modern cListDialog has no AddString
// API).
//
// 1:1 with legacy constants / enum:
//   * CHATLIST_TEXTLEN = 65
//   * 5 chat sheets: WHOLE=0, PARTY=1, GUILD=2, ALLIANCE=3,
//     SHOUT=4, MAX_CHAT_COUNT=5
//   * m_cPreWord[] per-sheet prefix (e.g. "!", "@", "#", "~")
//   * m_SelectedName[MAX_NAME_LENGTH+1]
//   * CHATLIST_TEXTEXTENT = 390 (TL locale; the modern port
//     keeps the 1:1 constant)

#pragma once

#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cwindow.hpp"

#include <cstdint>

namespace mxh::ui {

class cEditBox;
class cListDialog;
class cPushupButton;

// 1:1 with legacy chat sheet enum.
enum class ChatSheet : std::int32_t {
    Whole    = 0,
    Party    = 1,
    Guild    = 2,
    Alliance = 3,
    Shout    = 4,
    MaxCount = 5,
};

// 1:1 with legacy CHATLIST_TEXTLEN = 65.
inline constexpr std::int32_t kChatListTextLen = 65;

// 1:1 with legacy CHATLIST_TEXTEXTENT (TL locale).
inline constexpr std::int32_t kChatListTextExtent = 390;

// 1:1 with legacy MAX_NAME_LENGTH+1 = 17.
inline constexpr std::int32_t kMaxChatNameBuf = 17;

// 1:1 with legacy chat limit byte: each bit in the byte
// enables a sheet (WHOLE=1, PARTY=2, GUILD=4, ALLIANCE=8,
// SHOUT=16).  The legacy uses BYTE so the bitmask maxes at 8
// sheets, and the modern port keeps the same.
inline constexpr std::uint8_t kChatLimitWhole    = 1 << 0;
inline constexpr std::uint8_t kChatLimitParty    = 1 << 1;
inline constexpr std::uint8_t kChatLimitGuild    = 1 << 2;
inline constexpr std::uint8_t kChatLimitAlliance = 1 << 3;
inline constexpr std::uint8_t kChatLimitShout    = 1 << 4;

class cChatDialog : public cDialog {
public:
    cChatDialog();
    ~cChatDialog() override;

    cChatDialog(const cChatDialog&) = delete;
    cChatDialog& operator=(const cChatDialog&) = delete;

    // 1:1 with legacy Linking.  Wires cEditBox + 5
    // cListDialog + 5 cPushupButton + 1 AllShout button.
    void Linking();

    // 1:1 with legacy ActionEvent.
    std::uint32_t ActionEvent(/*CMouse**/ void* mouseInfo);

    // 1:1 with legacy AddMsg(BYTE ChatLimit, DWORD MsgColor,
    // char* str).  Routes the message to the matching sheet(s)
    // via the per-sheet ListAddItem callback.
    void AddMsg(std::uint8_t chatLimit, std::uint32_t msgColor, const char* str);

    // 1:1 with legacy AddMsgAll(DWORD MsgColor, char* str).
    // Routes to every sheet.
    void AddMsgAll(std::uint32_t msgColor, const char* str);

    // 1:1 with legacy OnActionEvent.
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // 1:1 with legacy SelectMenu(int nSheet).  Switches the
    // active sheet.
    void SelectMenu(int nSheet);

    // 1:1 with legacy SetEditBoxPreWord.  Stamps the
    // pre-word for the current sheet into the chat edit box.
    void SetEditBoxPreWord();

    // 1:1 with legacy IsPreWord(char c).  Returns true if
    // any sheet's pre-word matches c.
    bool IsPreWord(char c) const;

    // 1:1 with legacy HideChatDialog(BOOL).
    void HideChatDialog(bool bHide) noexcept { m_bHideChatDialog = bHide; }

    // 1:1 with legacy ShowGuildTab(BOOL).  Flips the
    // m_bShowGuildTab flag and (1:1) shows / hides the
    // guild + alliance pushup buttons.
    void ShowGuildTab(bool bShow) noexcept;

    // 1:1 with legacy GetSheetPosY / GetSheetHeight.
    std::uint16_t GetSheetPosY() const noexcept;
    std::uint16_t GetSheetHeight() const noexcept;

    // 1:1 with legacy SetAllShoutBtnPushed(BOOL).
    void SetAllShoutBtnPushed(bool val);

    // 1:1 with legacy GetSelectedName.
    const char* GetSelectedName() const noexcept { return m_SelectedName; }

    // 1:1 with legacy GetLineNum.
    int  GetLineNum() const noexcept;

    // 1:1 with legacy GetSheet(int nSheet).
    cListDialog*   GetSheet(int nSheet) const noexcept;
    cEditBox*      GetChatEditBox() const noexcept { return m_pChatEditBox; }
    int            GetCurSheetNum() const noexcept { return m_nCurSheetNum; }
    bool           isHideChatDialog() const noexcept { return m_bHideChatDialog; }
    bool           isShowGuildTab()   const noexcept { return m_bShowGuildTab; }
    char           PreWord(int sheet) const noexcept { return m_cPreWord[sheet < 0 || sheet >= kMaxChatCountNum ? 0 : sheet]; }
    int            chatListTextLen()   const noexcept { return kChatListTextLen; }
    int            chatListTextExtent() const noexcept { return kChatListTextExtent; }

    // Test hook -- inject the 5 sheets + 5 tab buttons + edit
    // box + all-shout button.
    struct ChildWindows {
        cEditBox*      chatEditBox  = nullptr;
        cListDialog*   sheets[5]     = {};
        cPushupButton* pbMenus[5]   = {};
        cPushupButton* allShout     = nullptr;
    };
    void SetChildWindowsForTest(const ChildWindows& w) { m_childWindows = w; }

    // Test hook -- inject a "sheet add item" callback (legacy
    // m_pSheet[i]->AddItem).
    using ListAddItemCallback = void(*)(int sheet,
                                        std::uint32_t color,
                                        const char* str,
                                        void* user);
    void SetListAddItemCallbackForTest(ListAddItemCallback cb, void* user) {
        m_listAddItemCb = cb; m_listAddItemUser = user;
    }

    // Test hook -- inject a "select menu" callback (legacy
    // m_pPBMenu[i]->SetPush).
    using SelectMenuCallback = void(*)(int sheet, void* user);
    void SetSelectMenuCallbackForTest(SelectMenuCallback cb, void* user) {
        m_selectMenuCb = cb; m_selectMenuUser = user;
    }

    // Test hook -- inject a "set edit text" callback (legacy
    // m_pChatEditBox->SetEditText).
    using SetEditTextCallback = void(*)(const char* text, void* user);
    void SetSetEditTextCallbackForTest(SetEditTextCallback cb, void* user) {
        m_setEditTextCb = cb; m_setEditTextUser = user;
    }

    // Test hook -- override the sheet line count (legacy
    // m_pSheet[i]->GetLineNum()).
    using LineCountCallback = int(*)(int sheet, void* user);
    void SetLineCountCallbackForTest(LineCountCallback cb, void* user) {
        m_lineCountCb = cb; m_lineCountUser = user;
    }

    // 1:1 with legacy pre-word init: WHOLE='!', PARTY='@',
    // GUILD='#', ALLIANCE='~', SHOUT='$'.  Tests can mutate
    // these via the public preWordForTest setter.
    void setPreWordForTest(int sheet, char c) {
        if (sheet < 0 || sheet >= kMaxChatCountNum) return;
        m_cPreWord[sheet] = c;
    }

    // 1:1 legacy default pre-words.
    static constexpr char kDefaultPreWordWhole    = '!';
    static constexpr char kDefaultPreWordParty    = '@';
    static constexpr char kDefaultPreWordGuild    = '#';
    static constexpr char kDefaultPreWordAlliance = '~';
    static constexpr char kDefaultPreWordShout    = '$';

    // 1:1 with legacy internal: array size for the sheets +
    // pbMenus + cPreWord arrays.
    static constexpr std::int32_t kMaxChatCountNum = 5;

private:
    cEditBox*      m_pChatEditBox  = nullptr;
    cListDialog*   m_pSheet[kMaxChatCountNum]   = {};
    cPushupButton* m_pPBMenu[kMaxChatCountNum]  = {};
    cPushupButton* m_pAllShout     = nullptr;
    int            m_nCurSheetNum   = 0;
    bool           m_bHideChatDialog = false;
    char           m_cPreWord[kMaxChatCountNum] = {
        kDefaultPreWordWhole,
        kDefaultPreWordParty,
        kDefaultPreWordGuild,
        kDefaultPreWordAlliance,
        kDefaultPreWordShout,
    };
    bool           m_bShowGuildTab = true;
    char           m_SelectedName[kMaxChatNameBuf] = {};

    ChildWindows   m_childWindows;
    ListAddItemCallback   m_listAddItemCb   = nullptr;
    void*                 m_listAddItemUser = nullptr;
    SelectMenuCallback    m_selectMenuCb    = nullptr;
    void*                 m_selectMenuUser  = nullptr;
    SetEditTextCallback   m_setEditTextCb   = nullptr;
    void*                 m_setEditTextUser = nullptr;
    LineCountCallback     m_lineCountCb     = nullptr;
    void*                 m_lineCountUser   = nullptr;
};

}  // namespace mxh::ui
