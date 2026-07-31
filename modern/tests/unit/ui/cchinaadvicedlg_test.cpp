//
// Unit tests for mxh::ui::cChinaAdviceDlg (Phase C dialog port).
//
// Locks down the 1:1 surface of legacy CChinaAdviceDlg
// (China-region advice / T&C dialog: 1 cTextArea +
// CHATMGR->GetChatMsg(30) text):
//   * Constants: kIdTextArea=360,
//                kChinaAdviceChatMsgId=30,
//                kWeBtnClick=0x0001
//   * Default construction: m_pTextArea null
//   * Inherits from cDialog
//   * NonCopyable
//   * Linking resolves 1 cTextArea child by id
//   * Linking without children leaves pointer null
//   * Linking is idempotent
//   * Linking before Init does not crash
//   * Linking with chat callback sets the resolved text
//   * Linking with chat callback that returns null uses placeholder
//   * Linking without chat callback uses placeholder
//   * Linking with no children does not crash
//   * Linking twice keeps the same pointer
//   * SetTextAreaForTest stores the pointer
//   * GetTextAreaForTest returns it
//   * Host-injected text area takes priority over Linking
//   * GetLastScriptTextForTest returns last text
//   * OnActionEvent is a no-op (legacy empty body)
//   * OnActionEvent before Linking does not crash
//   * OnActionEvent before Init does not crash
//   * OnActionEvent with WE_BTNCLICK is a no-op
//   * Multiple Linking calls keep the same text
//

#include "mxh/ui/cchinaadvicedlg.hpp"
#include "mxh/ui/cdialog.hpp"
#include "mxh/ui/ctextarea.hpp"
#include "mxh/ui/cwindow.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <memory>
#include <type_traits>

using mxh::ui::cChinaAdviceDlg;
using mxh::ui::cDialog;
using mxh::ui::cTextArea;
using mxh::ui::cWindow;

namespace {

// Chat message callback state.
const char* g_lastChatMsgIdResolved = nullptr;
int        g_lastChatMsgId          = -1;
int        g_chatMsgCallCount       = 0;
const char* g_chatMsgReturnValue     = "MockT&C_Text";

void ResetChatCbState() {
    g_lastChatMsgIdResolved = nullptr;
    g_lastChatMsgId          = -1;
    g_chatMsgCallCount       = 0;
    g_chatMsgReturnValue     = "MockT&C_Text";
}

const char* TestChatCallback(int chatMsgId, void* /*user*/) {
    g_lastChatMsgId          = chatMsgId;
    g_lastChatMsgIdResolved  = "MockT&C_Text";
    ++g_chatMsgCallCount;
    return g_chatMsgReturnValue;
}

const char* NullChatCallback(int /*chatMsgId*/, void* /*user*/) {
    ++g_chatMsgCallCount;
    return nullptr;
}

// Harness: dialog with pre-wired cTextArea child + chat callback.
struct Harness {
    cChinaAdviceDlg dlg;
    cTextArea       textArea;

    Harness() {
        dlg.Init(0, 0, 400, 300, nullptr, 0);
        textArea.Init(0, 0, 380, 280, nullptr,
                      cChinaAdviceDlg::kIdTextArea);
        dlg.SetTextAreaForTest(&textArea);
        dlg.SetChatMsgCallbackForTest(TestChatCallback, nullptr);
        ResetChatCbState();
    }
};

}  // namespace

// ---------- Construction / destruction ----------

TEST(CChinaAdviceDlgTest, CtorDoesNotCrash) {
    cChinaAdviceDlg dlg;
    SUCCEED();
}

TEST(CChinaAdviceDlgTest, DtorDoesNotCrash) {
    cChinaAdviceDlg dlg;
    SUCCEED();
}

TEST(CChinaAdviceDlgTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cChinaAdviceDlg>,
                  "cChinaAdviceDlg must inherit from cDialog");
    SUCCEED();
}

TEST(CChinaAdviceDlgTest, NonCopyable) {
    static_assert(!std::is_copy_constructible_v<cChinaAdviceDlg>,
                  "cChinaAdviceDlg must be non-copyable");
    static_assert(!std::is_copy_assignable_v<cChinaAdviceDlg>,
                  "cChinaAdviceDlg must be non-copy-assignable");
    SUCCEED();
}

// ---------- Constants ----------

TEST(CChinaAdviceDlgTest, IdConstantMatchesExpectedLocalRange) {
    EXPECT_EQ(cChinaAdviceDlg::kIdTextArea, 360);
}

TEST(CChinaAdviceDlgTest, ChatMsgIdConstantMatchesLegacy) {
    EXPECT_EQ(cChinaAdviceDlg::kChinaAdviceChatMsgId, 30);
}

TEST(CChinaAdviceDlgTest, WeBtnClickConstantMatchesLegacy) {
    EXPECT_EQ(cChinaAdviceDlg::kWeBtnClick, 0x0001u);
}

TEST(CChinaAdviceDlgTest, PlaceholderTextIsSet) {
    EXPECT_NE(cChinaAdviceDlg::kPlaceholderText, nullptr);
    EXPECT_GT(std::char_traits<char>::length(cChinaAdviceDlg::kPlaceholderText), 0u);
}

// ---------- Default state ----------

TEST(CChinaAdviceDlgTest, DefaultTextAreaIsNull) {
    cChinaAdviceDlg dlg;
    EXPECT_EQ(dlg.GetTextAreaForTest(), nullptr);
}

TEST(CChinaAdviceDlgTest, DefaultLastScriptTextIsNull) {
    cChinaAdviceDlg dlg;
    EXPECT_EQ(dlg.GetLastScriptTextForTest(), nullptr);
}

// ---------- Linking: cTextArea resolution ----------

TEST(CChinaAdviceDlgTest, LinkingResolvesTextAreaFromTree) {
    cChinaAdviceDlg dlg;
    dlg.Init(0, 0, 400, 300, nullptr, 0);
    auto ta = std::make_unique<cTextArea>();
    ta->Init(0, 0, 380, 280, nullptr,
             cChinaAdviceDlg::kIdTextArea);
    cTextArea* taPtr = ta.get();
    dlg.Add(std::move(ta));
    dlg.Linking();
    EXPECT_EQ(dlg.GetTextAreaForTest(), taPtr);
}

TEST(CChinaAdviceDlgTest, LinkingWithoutChildrenLeavesPointerNull) {
    cChinaAdviceDlg dlg;
    dlg.Init(0, 0, 400, 300, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetTextAreaForTest(), nullptr);
}

TEST(CChinaAdviceDlgTest, LinkingIsIdempotent) {
    Harness h;
    h.dlg.Linking();
    h.dlg.Linking();
    EXPECT_EQ(h.dlg.GetTextAreaForTest(), &h.textArea);
}

TEST(CChinaAdviceDlgTest, LinkingBeforeInitDoesNotCrash) {
    cChinaAdviceDlg dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CChinaAdviceDlgTest, HostInjectedTakesPriorityOverLinking) {
    cChinaAdviceDlg dlg;
    dlg.Init(0, 0, 400, 300, nullptr, 0);
    cTextArea ta;
    ta.Init(0, 0, 380, 280, nullptr,
            cChinaAdviceDlg::kIdTextArea);
    dlg.SetTextAreaForTest(&ta);
    dlg.Linking();
    EXPECT_EQ(dlg.GetTextAreaForTest(), &ta);
}

// ---------- Linking: script text ----------

TEST(CChinaAdviceDlgTest, LinkingSetsScriptTextViaCallback) {
    Harness h;
    h.dlg.Linking();
    EXPECT_STREQ(h.dlg.GetLastScriptTextForTest(), "MockT&C_Text");
}

TEST(CChinaAdviceDlgTest, LinkingCallsCallbackWithChatMsgId30) {
    Harness h;
    h.dlg.Linking();
    EXPECT_EQ(g_lastChatMsgId, cChinaAdviceDlg::kChinaAdviceChatMsgId);
    EXPECT_EQ(g_chatMsgCallCount, 1);
}

TEST(CChinaAdviceDlgTest, LinkingWithoutCallbackUsesPlaceholder) {
    cChinaAdviceDlg dlg;
    dlg.Init(0, 0, 400, 300, nullptr, 0);
    cTextArea ta;
    ta.Init(0, 0, 380, 280, nullptr,
            cChinaAdviceDlg::kIdTextArea);
    dlg.SetTextAreaForTest(&ta);
    dlg.Linking();
    EXPECT_STREQ(dlg.GetLastScriptTextForTest(),
                 cChinaAdviceDlg::kPlaceholderText);
}

TEST(CChinaAdviceDlgTest, LinkingWithNullCallbackResultUsesPlaceholder) {
    cChinaAdviceDlg dlg;
    dlg.Init(0, 0, 400, 300, nullptr, 0);
    cTextArea ta;
    ta.Init(0, 0, 380, 280, nullptr,
            cChinaAdviceDlg::kIdTextArea);
    dlg.SetTextAreaForTest(&ta);
    dlg.SetChatMsgCallbackForTest(NullChatCallback, nullptr);
    dlg.Linking();
    EXPECT_STREQ(dlg.GetLastScriptTextForTest(),
                 cChinaAdviceDlg::kPlaceholderText);
}

TEST(CChinaAdviceDlgTest, LinkingWithoutTextAreaDoesNotSetScriptText) {
    ResetChatCbState();
    cChinaAdviceDlg dlg;
    dlg.Init(0, 0, 400, 300, nullptr, 0);
    dlg.SetChatMsgCallbackForTest(TestChatCallback, nullptr);
    dlg.Linking();
    // No text area, so no SetScriptText was called.
    EXPECT_EQ(dlg.GetLastScriptTextForTest(), nullptr);
    // Callback was not called either.
    EXPECT_EQ(g_chatMsgCallCount, 0);
}

TEST(CChinaAdviceDlgTest, MultipleLinkingCallsKeepLastText) {
    Harness h;
    h.dlg.Linking();
    EXPECT_STREQ(h.dlg.GetLastScriptTextForTest(), "MockT&C_Text");
    g_chatMsgReturnValue = "UpdatedT&C";
    h.dlg.Linking();
    EXPECT_STREQ(h.dlg.GetLastScriptTextForTest(), "UpdatedT&C");
    EXPECT_EQ(g_chatMsgCallCount, 2);
}

// ---------- OnActionEvent: empty no-op ----------

TEST(CChinaAdviceDlgTest, OnActionEventIsNoOp) {
    Harness h;
    // 1:1 with legacy empty body. No state change.
    h.dlg.OnActionEvent(0, nullptr, 0);
    h.dlg.OnActionEvent(360, &h.textArea, cChinaAdviceDlg::kWeBtnClick);
    h.dlg.OnActionEvent(9999, nullptr, 0xFFFFu);
    SUCCEED();
}

TEST(CChinaAdviceDlgTest, OnActionEventDoesNotChangeScriptText) {
    Harness h;
    h.dlg.Linking();
    const char* before = h.dlg.GetLastScriptTextForTest();
    h.dlg.OnActionEvent(cChinaAdviceDlg::kIdTextArea, &h.textArea,
                        cChinaAdviceDlg::kWeBtnClick);
    EXPECT_EQ(h.dlg.GetLastScriptTextForTest(), before);
}

TEST(CChinaAdviceDlgTest, OnActionEventBeforeLinkingDoesNotCrash) {
    cChinaAdviceDlg dlg;
    dlg.Init(0, 0, 400, 300, nullptr, 0);
    dlg.OnActionEvent(cChinaAdviceDlg::kIdTextArea, nullptr, 0);
    SUCCEED();
}

TEST(CChinaAdviceDlgTest, OnActionEventBeforeInitDoesNotCrash) {
    cChinaAdviceDlg dlg;
    dlg.OnActionEvent(0, nullptr, 0);
    SUCCEED();
}

TEST(CChinaAdviceDlgTest, OnActionEventWithBtnClickIsNoOp) {
    Harness h;
    h.dlg.Linking();
    int before = g_chatMsgCallCount;
    h.dlg.OnActionEvent(cChinaAdviceDlg::kIdTextArea, &h.textArea,
                        cChinaAdviceDlg::kWeBtnClick);
    // OnActionEvent must not call chat callback.
    EXPECT_EQ(g_chatMsgCallCount, before);
}