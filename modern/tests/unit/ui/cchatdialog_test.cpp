// mxh/tests/unit/ui/cchatdialog_test.cpp
//
// Unit tests for mxh::ui::cChatDialog (Phase C dialog port).
//
// Locks down the 1:1 surface:
//   * ChatSheet enum: Whole=0, Party=1, Guild=2, Alliance=3,
//     Shout=4, MaxCount=5
//   * kChatListTextLen == 65, kChatListTextExtent == 390
//   * kChatLimit* bit positions
//   * Default pre-words: '!' / '@' / '#' / '~' / '$'
//   * AddMsg(chatLimit, color, str) routes to matching sheets
//   * AddMsgAll routes to all 5 sheets
//   * SelectMenu switches the current sheet + fires the
//     select-menu callback + stamps the pre-word into the
//     edit box
//   * SetEditBoxPreWord writes the current pre-word
//   * IsPreWord matches across all 5 sheets
//   * HideChatDialog / ShowGuildTab / SetAllShoutBtnPushed
//     are 1:1 surface flags
//   * GetLineNum sums the per-sheet line counts
//   * GetSheet returns the bound cListDialog* by index
//   * GetChatEditBox returns the bound edit box
//   * NonCopyable

#include "mxh/ui/cchatdialog.hpp"
#include "../../../src/ui/legacy_window_event.hpp"
#include "mxh/ui/ceditbox.hpp"
#include "mxh/ui/clistdialog.hpp"
#include "mxh/ui/cPushupButton.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

using mxh::ui::cChatDialog;
using mxh::ui::ChatSheet;
using mxh::ui::cEditBox;
using mxh::ui::cListDialog;
using mxh::ui::cPushupButton;

namespace test_chatdlg {

struct CapturedAddItem {
    int sheet;
    std::uint32_t color;
    std::string str;
};

int g_addItemCount = 0;
std::vector<CapturedAddItem> g_captured;
std::int32_t g_lastSelectSheet = -1;
int g_selectMenuCount = 0;
char g_lastEditText[8] = {};
int g_setEditTextCount = 0;
std::vector<int> g_sheetLineCounts;

void faAddItem(int sheet, std::uint32_t color, const char* str, void* /*user*/) {
    ++g_addItemCount;
    g_captured.push_back({sheet, color, str ? str : ""});
}
void faSelectMenu(int sheet, void* /*user*/) {
    ++g_selectMenuCount;
    g_lastSelectSheet = sheet;
}
void faSetEditText(const char* text, void* /*user*/) {
    ++g_setEditTextCount;
    std::strncpy(g_lastEditText, text ? text : "", sizeof(g_lastEditText) - 1);
    g_lastEditText[sizeof(g_lastEditText) - 1] = '\0';
}
int faLineCount(int sheet, void* /*user*/) {
    if (sheet < 0 || sheet >= static_cast<int>(g_sheetLineCounts.size())) return 0;
    return g_sheetLineCounts[sheet];
}

}  // namespace test_chatdlg

TEST(CChatDialog, ChatSheetEnumIsStable) {
    EXPECT_EQ(static_cast<int>(ChatSheet::Whole),    0);
    EXPECT_EQ(static_cast<int>(ChatSheet::Party),    1);
    EXPECT_EQ(static_cast<int>(ChatSheet::Guild),    2);
    EXPECT_EQ(static_cast<int>(ChatSheet::Alliance), 3);
    EXPECT_EQ(static_cast<int>(ChatSheet::Shout),    4);
    EXPECT_EQ(static_cast<int>(ChatSheet::MaxCount), 5);
}

TEST(CChatDialog, ChatLimitBitPositions) {
    EXPECT_EQ(mxh::ui::kChatLimitWhole,    0x01);
    EXPECT_EQ(mxh::ui::kChatLimitParty,    0x02);
    EXPECT_EQ(mxh::ui::kChatLimitGuild,    0x04);
    EXPECT_EQ(mxh::ui::kChatLimitAlliance, 0x08);
    EXPECT_EQ(mxh::ui::kChatLimitShout,    0x10);
}

TEST(CChatDialog, ConstantsMatchLegacy) {
    EXPECT_EQ(mxh::ui::kChatListTextLen, 65);
    EXPECT_EQ(mxh::ui::kChatListTextExtent, 390);
    EXPECT_EQ(cChatDialog::kMaxChatCountNum, 5);
    EXPECT_EQ(mxh::ui::kMaxChatNameBuf, 17);
}

TEST(CChatDialog, DefaultPreWordsMatchLegacy) {
    EXPECT_EQ(cChatDialog::kDefaultPreWordWhole,    '!');
    EXPECT_EQ(cChatDialog::kDefaultPreWordParty,    '@');
    EXPECT_EQ(cChatDialog::kDefaultPreWordGuild,    '#');
    EXPECT_EQ(cChatDialog::kDefaultPreWordAlliance, '~');
    EXPECT_EQ(cChatDialog::kDefaultPreWordShout,    '$');
}

TEST(CChatDialog, DefaultConstructionHasCorrectPreWords) {
    cChatDialog d;
    EXPECT_EQ(d.PreWord(static_cast<int>(ChatSheet::Whole)),    '!');
    EXPECT_EQ(d.PreWord(static_cast<int>(ChatSheet::Party)),    '@');
    EXPECT_EQ(d.PreWord(static_cast<int>(ChatSheet::Guild)),    '#');
    EXPECT_EQ(d.PreWord(static_cast<int>(ChatSheet::Alliance)), '~');
    EXPECT_EQ(d.PreWord(static_cast<int>(ChatSheet::Shout)),    '$');
    EXPECT_EQ(d.GetCurSheetNum(), 0);
    EXPECT_FALSE(d.isHideChatDialog());
    EXPECT_TRUE(d.isShowGuildTab());
}

TEST(CChatDialog, SetPreWordForTestStores) {
    cChatDialog d;
    d.setPreWordForTest(0, '?');
    EXPECT_EQ(d.PreWord(0), '?');
    d.setPreWordForTest(99, '!');   // out of range
    EXPECT_EQ(d.PreWord(0), '?');
}

TEST(CChatDialog, LinkingWiresChildren) {
    cChatDialog d;
    cEditBox editBox;
    cListDialog sheets[5];
    cPushupButton pbMenus[5];
    cPushupButton allShout;
    cChatDialog::ChildWindows w{};
    w.chatEditBox = &editBox;
    for (int i = 0; i < 5; ++i) { w.sheets[i] = &sheets[i]; w.pbMenus[i] = &pbMenus[i]; }
    w.allShout = &allShout;
    d.SetChildWindowsForTest(w);
    d.Linking();
    EXPECT_EQ(d.GetChatEditBox(), &editBox);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(d.GetSheet(i), &sheets[i]);
    }
}

TEST(CChatDialog, AddMsgRoutesToMatchingSheets) {
    test_chatdlg::g_addItemCount = 0;
    test_chatdlg::g_captured.clear();
    cChatDialog d;
    d.SetListAddItemCallbackForTest(&test_chatdlg::faAddItem, nullptr);
    d.AddMsg(mxh::ui::kChatLimitParty | mxh::ui::kChatLimitShout, 0xFF00FFu, "hello");
    EXPECT_EQ(test_chatdlg::g_addItemCount, 2);
    ASSERT_EQ(test_chatdlg::g_captured.size(), 2u);
    EXPECT_EQ(test_chatdlg::g_captured[0].sheet, static_cast<int>(ChatSheet::Party));
    EXPECT_EQ(test_chatdlg::g_captured[0].color, 0xFF00FFu);
    EXPECT_EQ(test_chatdlg::g_captured[0].str, "hello");
    EXPECT_EQ(test_chatdlg::g_captured[1].sheet, static_cast<int>(ChatSheet::Shout));
}

TEST(CChatDialog, AddMsgAllRoutesToAllSheets) {
    test_chatdlg::g_addItemCount = 0;
    test_chatdlg::g_captured.clear();
    cChatDialog d;
    d.SetListAddItemCallbackForTest(&test_chatdlg::faAddItem, nullptr);
    d.AddMsgAll(0x11223344u, "broadcast");
    EXPECT_EQ(test_chatdlg::g_addItemCount, 5);
    // 5 sheets, all with the same color + string.
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(test_chatdlg::g_captured[i].color, 0x11223344u);
        EXPECT_EQ(test_chatdlg::g_captured[i].str, "broadcast");
    }
}

TEST(CChatDialog, AddMsgNullStringIsSafe) {
    test_chatdlg::g_addItemCount = 0;
    test_chatdlg::g_captured.clear();
    cChatDialog d;
    d.SetListAddItemCallbackForTest(&test_chatdlg::faAddItem, nullptr);
    d.AddMsg(mxh::ui::kChatLimitWhole, 0, nullptr);
    // No crash, no add.
    EXPECT_EQ(test_chatdlg::g_addItemCount, 0);
}

TEST(CChatDialog, AddMsgWithoutCallbackIsSafe) {
    cChatDialog d;
    d.AddMsg(mxh::ui::kChatLimitWhole, 0, "x");
    SUCCEED();
}

TEST(CChatDialog, SelectMenuSwitchesSheetAndFiresCallback) {
    test_chatdlg::g_lastSelectSheet = -1;
    test_chatdlg::g_selectMenuCount = 0;
    test_chatdlg::g_setEditTextCount = 0;
    cChatDialog d;
    d.SetSelectMenuCallbackForTest(&test_chatdlg::faSelectMenu, nullptr);
    d.SetSetEditTextCallbackForTest(&test_chatdlg::faSetEditText, nullptr);
    d.SelectMenu(static_cast<int>(ChatSheet::Guild));
    EXPECT_EQ(d.GetCurSheetNum(), static_cast<int>(ChatSheet::Guild));
    EXPECT_EQ(test_chatdlg::g_lastSelectSheet, static_cast<int>(ChatSheet::Guild));
    EXPECT_EQ(test_chatdlg::g_selectMenuCount, 1);
    EXPECT_GT(test_chatdlg::g_setEditTextCount, 0);
    EXPECT_EQ(test_chatdlg::g_lastEditText[0], '#');
}

TEST(CChatDialog, SelectMenuOutOfRangeIsNoOp) {
    test_chatdlg::g_selectMenuCount = 0;
    test_chatdlg::g_lastSelectSheet = -1;
    cChatDialog d;
    d.SetSelectMenuCallbackForTest(&test_chatdlg::faSelectMenu, nullptr);
    d.SelectMenu(-1);
    EXPECT_EQ(d.GetCurSheetNum(), 0);
    d.SelectMenu(99);
    EXPECT_EQ(d.GetCurSheetNum(), 0);
    EXPECT_EQ(test_chatdlg::g_selectMenuCount, 0);
}

TEST(CChatDialog, OnActionEventTabClickSelectsMenu) {
    test_chatdlg::g_selectMenuCount = 0;
    cChatDialog d;
    d.SetSelectMenuCallbackForTest(&test_chatdlg::faSelectMenu, nullptr);
    d.SetSetEditTextCallbackForTest(&test_chatdlg::faSetEditText, nullptr);
    d.OnActionEvent(static_cast<int>(ChatSheet::Party), nullptr, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(d.GetCurSheetNum(), static_cast<int>(ChatSheet::Party));
    EXPECT_EQ(test_chatdlg::g_selectMenuCount, 1);
}

TEST(CChatDialog, OnActionEventIgnoresNonClickEvents) {
    test_chatdlg::g_selectMenuCount = 0;
    cChatDialog d;
    d.SetSelectMenuCallbackForTest(&test_chatdlg::faSelectMenu, nullptr);
    d.OnActionEvent(1, nullptr, 0x0000);
    EXPECT_EQ(d.GetCurSheetNum(), 0);
    EXPECT_EQ(test_chatdlg::g_selectMenuCount, 0);
}

TEST(CChatDialog, OnActionEventIgnoresOutOfRangeId) {
    cChatDialog d;
    d.SetSelectMenuCallbackForTest(&test_chatdlg::faSelectMenu, nullptr);
    d.OnActionEvent(99, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(d.GetCurSheetNum(), 0);
}

TEST(CChatDialog, SetEditBoxPreWordStampsCurrentPreWord) {
    test_chatdlg::g_setEditTextCount = 0;
    cChatDialog d;
    d.SetSetEditTextCallbackForTest(&test_chatdlg::faSetEditText, nullptr);
    d.SelectMenu(static_cast<int>(ChatSheet::Shout));
    EXPECT_GT(test_chatdlg::g_setEditTextCount, 0);
    EXPECT_EQ(test_chatdlg::g_lastEditText[0], '$');
}

TEST(CChatDialog, IsPreWordMatchesAnySheet) {
    cChatDialog d;
    EXPECT_TRUE(d.IsPreWord('!'));
    EXPECT_TRUE(d.IsPreWord('@'));
    EXPECT_TRUE(d.IsPreWord('#'));
    EXPECT_TRUE(d.IsPreWord('~'));
    EXPECT_TRUE(d.IsPreWord('$'));
    EXPECT_FALSE(d.IsPreWord('?'));
    EXPECT_FALSE(d.IsPreWord('\0'));
}

TEST(CChatDialog, IsPreWordMatchesCustomPreWord) {
    cChatDialog d;
    d.setPreWordForTest(0, '?');
    EXPECT_TRUE(d.IsPreWord('?'));
    EXPECT_FALSE(d.IsPreWord('!'));
}

TEST(CChatDialog, HideChatDialogStoresFlag) {
    cChatDialog d;
    d.HideChatDialog(true);
    EXPECT_TRUE(d.isHideChatDialog());
    d.HideChatDialog(false);
    EXPECT_FALSE(d.isHideChatDialog());
}

TEST(CChatDialog, ShowGuildTabStoresFlag) {
    cChatDialog d;
    d.ShowGuildTab(false);
    EXPECT_FALSE(d.isShowGuildTab());
    d.ShowGuildTab(true);
    EXPECT_TRUE(d.isShowGuildTab());
}

TEST(CChatDialog, SetAllShoutBtnPushedTolerated) {
    cChatDialog d;
    d.SetAllShoutBtnPushed(true);
    d.SetAllShoutBtnPushed(false);
    SUCCEED();
}

TEST(CChatDialog, GetLineNumSumsPerSheet) {
    test_chatdlg::g_sheetLineCounts = {1, 2, 3, 4, 5};
    cChatDialog d;
    d.SetLineCountCallbackForTest(&test_chatdlg::faLineCount, nullptr);
    EXPECT_EQ(d.GetLineNum(), 15);
}

TEST(CChatDialog, GetSheetOutOfRangeIsNull) {
    cChatDialog d;
    EXPECT_EQ(d.GetSheet(-1), nullptr);
    EXPECT_EQ(d.GetSheet(99), nullptr);
}

TEST(CChatDialog, GetSelectedNameStartsEmpty) {
    cChatDialog d;
    EXPECT_STREQ(d.GetSelectedName(), "");
}

TEST(CChatDialog, ActionEventIsNoOpStub) {
    cChatDialog d;
    EXPECT_EQ(d.ActionEvent(nullptr), 0u);
}

TEST(CChatDialog, NonCopyable) {
    static_assert(!std::is_copy_constructible_v<cChatDialog>);
    static_assert(!std::is_copy_assignable_v<cChatDialog>);
}
