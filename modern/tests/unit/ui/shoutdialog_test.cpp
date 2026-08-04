// shoutdialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cShoutDialog (shout message sender
// dialog: 1 cEditBox + 1 SEND button + item info state).
//
// Covers modern/src/ui/shoutdialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\ShoutDialog.h (832 B) and
//   墨香【源码】\[Client]MH\ShoutDialog.cpp.
//
// What's tested:
//   - Default construction: cShoutDialog is a cDialog
//     and inherits its tree management.
//   - 2 state fields start 0 (1:1 with legacy
//     default init).
//   - Id constant matches expected local range
//     (kIdMsgBox=410).
//   - Linking resolves the cEditBox child by id.
//   - Linking without children leaves m_pMsgBox
//     null (SendShoutMsgSyn is safe).
//   - Linking before Init does not crash.
//   - SetItemInfo round-trip (item idx + pos).
//   - SendShoutMsgSyn preserves the legacy empty,
//     filtered, successful-send, and reset paths via
//     optional host callbacks.
//   - SendShoutMsgSyn without Linking is safe.
//   - SendShoutMsgSyn before Init is safe.
//
// 1:1 quirks preserved:
//   - Ctor body empty (1:1 quirk: m_type =
//     WT_SHOUT_DLG drop, modern cWindow does not
//     have m_type field).
//   - State fields default 0 (1:1 with legacy
//     m_dwItemIdx = m_dwItemPos = 0 init).
//   - SetItemInfo inline setter (1:1 with legacy
//     inline setter).
//   - SendShoutMsgSyn keeps the legacy callback
//     ordering, formatting, WORD casts, and resets.
//   - Local id range 410 (distinct from 200-401
//     used by previous Tier 2 dialogs; no
//     collision).

#include "shoutdialog.hpp"
#include "cdialog.hpp"
#include "ceditbox.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

namespace mxh::ui::test {

// ===========================================================================
// Construction + state
// ===========================================================================

TEST(CShoutDialogTest, DefaultConstructionHasZeroState) {
    cShoutDialog dlg;
    // 1:1 quirk: ctor body is empty (legacy
    // m_type = WT_SHOUT_DLG drop, modern cWindow
    // does not have m_type field). m_dwItemIdx +
    // m_dwItemPos start at 0.
    EXPECT_EQ(dlg.GetItemIdx(), 0u);
    EXPECT_EQ(dlg.GetItemPos(), 0u);
}

TEST(CShoutDialogTest, InheritsDialogTreeManagement) {
    cShoutDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CShoutDialogTest, IdConstantMatchesExpectedLocalRange) {
    EXPECT_EQ(cShoutDialog::kIdMsgBox, 410);
}

// ===========================================================================
// Linking
// ===========================================================================

TEST(CShoutDialogTest, LinkingResolvesEditBox) {
    cShoutDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 200, 14, nullptr, nullptr, cShoutDialog::kIdMsgBox);
    edit->InitEditbox(50, 64);
    cEditBox* pEdit = edit.get();
    dlg.Add(std::unique_ptr<cWindow>(edit.release()));

    dlg.Linking();
    // m_pMsgBox is private; verified indirectly
    // via the Linking call not crashing + the
    // dialog state is consistent.
    pEdit->SetEditText("hello world");
    EXPECT_EQ(pEdit->editText(), "hello world");
}

TEST(CShoutDialogTest, LinkingWithoutChildrenDoesNotCrash) {
    cShoutDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    // SendShoutMsgSyn without m_pMsgBox is safe.
    dlg.SendShoutMsgSyn();
    SUCCEED();
}

TEST(CShoutDialogTest, LinkingBeforeInitDoesNotCrash) {
    cShoutDialog dlg;
    dlg.Linking();
    SUCCEED();
}

// ===========================================================================
// SetItemInfo
// ===========================================================================

TEST(CShoutDialogTest, SetItemInfoRoundTrip) {
    cShoutDialog dlg;
    dlg.SetItemInfo(42u, 100u);
    EXPECT_EQ(dlg.GetItemIdx(), 42u);
    EXPECT_EQ(dlg.GetItemPos(), 100u);
}

TEST(CShoutDialogTest, SetItemInfoOverridesPrevious) {
    cShoutDialog dlg;
    dlg.SetItemInfo(1u, 2u);
    dlg.SetItemInfo(100u, 200u);
    EXPECT_EQ(dlg.GetItemIdx(), 100u);
    EXPECT_EQ(dlg.GetItemPos(), 200u);
}

TEST(CShoutDialogTest, SetItemInfoZeroIsValid) {
    cShoutDialog dlg;
    dlg.SetItemInfo(0u, 0u);
    EXPECT_EQ(dlg.GetItemIdx(), 0u);
    EXPECT_EQ(dlg.GetItemPos(), 0u);
}

// ===========================================================================
// SendShoutMsgSyn
// ===========================================================================

namespace {

struct ShoutChildren {
    cEditBox* edit = nullptr;
};

void BuildShoutDialog(cShoutDialog& dlg, ShoutChildren& children) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 200, 14, nullptr, nullptr, cShoutDialog::kIdMsgBox);
    edit->InitEditbox(100, 128);
    children.edit = edit.get();
    dlg.Add(std::unique_ptr<cWindow>(edit.release()));
    dlg.Linking();
}

struct ShoutCallbackState {
    bool filtered = false;
    std::string heroName = "Hero";
    std::uint32_t heroObjectId = 77;
    int messageCalls = 0;
    std::int32_t lastMessageId = 0;
    int filterCalls = 0;
    std::string filteredText;
    int sendCalls = 0;
    std::uint32_t sentObjectId = 0;
    std::uint16_t sentItemIdx = 0;
    std::uint16_t sentItemPos = 0;
    std::string sentMessage;
};

void AddShoutSystemMessage(std::int32_t messageId, void* userData) {
    auto& state = *static_cast<ShoutCallbackState*>(userData);
    ++state.messageCalls;
    state.lastMessageId = messageId;
}

bool FilterShoutChat(const char* message, void* userData) {
    auto& state = *static_cast<ShoutCallbackState*>(userData);
    ++state.filterCalls;
    state.filteredText = message ? message : "";
    return state.filtered;
}

const char* GetShoutHeroName(void* userData) {
    return static_cast<ShoutCallbackState*>(userData)->heroName.c_str();
}

std::uint32_t GetShoutHeroObjectId(void* userData) {
    return static_cast<ShoutCallbackState*>(userData)->heroObjectId;
}

void SendShout(std::uint32_t objectId, std::uint16_t itemIdx,
               std::uint16_t itemPos, const char* message, void* userData) {
    auto& state = *static_cast<ShoutCallbackState*>(userData);
    ++state.sendCalls;
    state.sentObjectId = objectId;
    state.sentItemIdx = itemIdx;
    state.sentItemPos = itemPos;
    state.sentMessage = message ? message : "";
}

void InstallShoutCallbacks(cShoutDialog& dlg, ShoutCallbackState& state) {
    dlg.SetCallbacks(AddShoutSystemMessage, FilterShoutChat,
                     GetShoutHeroName, GetShoutHeroObjectId,
                     SendShout, &state);
}

}  // namespace

TEST(CShoutDialogTest, LegacyConstantsMatchSource) {
    EXPECT_EQ(cShoutDialog::kMaxShoutLength, 60u);
    EXPECT_EQ(cShoutDialog::kEmptyMessageId, 903);
    EXPECT_EQ(cShoutDialog::kFilteredMessageId, 27);
}

TEST(CShoutDialogTest, EmptyMessageShows903WithoutClearingItemState) {
    cShoutDialog dlg;
    ShoutChildren children;
    BuildShoutDialog(dlg, children);
    ShoutCallbackState state;
    InstallShoutCallbacks(dlg, state);
    dlg.SetItemInfo(42, 100);

    EXPECT_FALSE(dlg.SendShoutMsgSyn());

    EXPECT_EQ(state.lastMessageId, 903);
    EXPECT_EQ(state.filterCalls, 0);
    EXPECT_EQ(dlg.GetItemIdx(), 42u);
    EXPECT_EQ(dlg.GetItemPos(), 100u);
}

TEST(CShoutDialogTest, NonEmptyMessageClearsEditBeforeFilterFailure) {
    cShoutDialog dlg;
    ShoutChildren children;
    BuildShoutDialog(dlg, children);
    ShoutCallbackState state;
    state.filtered = true;
    InstallShoutCallbacks(dlg, state);
    dlg.SetItemInfo(42, 100);
    children.edit->SetEditText("blocked shout");

    EXPECT_FALSE(dlg.SendShoutMsgSyn());

    EXPECT_TRUE(children.edit->editText().empty());
    EXPECT_EQ(state.filteredText, "blocked shout");
    EXPECT_EQ(state.lastMessageId, 27);
    EXPECT_EQ(state.sendCalls, 0);
    EXPECT_EQ(dlg.GetItemIdx(), 42u);
    EXPECT_EQ(dlg.GetItemPos(), 100u);
}

TEST(CShoutDialogTest, SuccessFormatsMessageSendsAndResetsState) {
    cShoutDialog dlg;
    ShoutChildren children;
    BuildShoutDialog(dlg, children);
    ShoutCallbackState state;
    InstallShoutCallbacks(dlg, state);
    dlg.SetItemInfo(42, 100);
    dlg.SetActive(true);
    children.edit->SetEditText("hello world");

    EXPECT_TRUE(dlg.SendShoutMsgSyn());

    EXPECT_EQ(state.sendCalls, 1);
    EXPECT_EQ(state.sentObjectId, 77u);
    EXPECT_EQ(state.sentItemIdx, 42u);
    EXPECT_EQ(state.sentItemPos, 100u);
    EXPECT_EQ(state.sentMessage, "Hero : hello world");
    EXPECT_FALSE(dlg.isActive());
    EXPECT_EQ(dlg.GetItemIdx(), 0u);
    EXPECT_EQ(dlg.GetItemPos(), 0u);
}

TEST(CShoutDialogTest, SuccessTruncatesItemFieldsToLegacyWord) {
    cShoutDialog dlg;
    ShoutChildren children;
    BuildShoutDialog(dlg, children);
    ShoutCallbackState state;
    InstallShoutCallbacks(dlg, state);
    dlg.SetItemInfo(0x12345u, 0x23456u);
    children.edit->SetEditText("hello");

    EXPECT_TRUE(dlg.SendShoutMsgSyn());

    EXPECT_EQ(state.sentItemIdx, 0x2345u);
    EXPECT_EQ(state.sentItemPos, 0x3456u);
}

TEST(CShoutDialogTest, FormattedMessageIsBoundedToLegacyBuffer) {
    cShoutDialog dlg;
    ShoutChildren children;
    BuildShoutDialog(dlg, children);
    ShoutCallbackState state;
    state.heroName = "VeryLongHeroName";
    InstallShoutCallbacks(dlg, state);
    children.edit->SetEditText(std::string(60, 'x'));

    EXPECT_TRUE(dlg.SendShoutMsgSyn());

    EXPECT_LE(state.sentMessage.size(), cShoutDialog::kMaxShoutLength);
    EXPECT_EQ(state.sentMessage.substr(0, 19), "VeryLongHeroName : ");
}

TEST(CShoutDialogTest, MissingSendCallbackReturnsFalseAfterClearingEdit) {
    cShoutDialog dlg;
    ShoutChildren children;
    BuildShoutDialog(dlg, children);
    ShoutCallbackState state;
    dlg.SetCallbacks(AddShoutSystemMessage, FilterShoutChat,
                     GetShoutHeroName, GetShoutHeroObjectId,
                     nullptr, &state);
    dlg.SetItemInfo(4, 5);
    children.edit->SetEditText("hello");

    EXPECT_FALSE(dlg.SendShoutMsgSyn());

    EXPECT_TRUE(children.edit->editText().empty());
    EXPECT_EQ(dlg.GetItemIdx(), 4u);
    EXPECT_EQ(dlg.GetItemPos(), 5u);
}

TEST(CShoutDialogTest, SetCallbacksReplacesExistingDispatch) {
    cShoutDialog dlg;
    ShoutChildren children;
    BuildShoutDialog(dlg, children);
    ShoutCallbackState firstState;
    ShoutCallbackState secondState;
    InstallShoutCallbacks(dlg, firstState);
    InstallShoutCallbacks(dlg, secondState);
    children.edit->SetEditText("hello");

    EXPECT_TRUE(dlg.SendShoutMsgSyn());

    EXPECT_EQ(firstState.sendCalls, 0);
    EXPECT_EQ(secondState.sendCalls, 1);
}

TEST(CShoutDialogTest, SendShoutMsgSynWithoutLinkIsSafe) {
    cShoutDialog dlg;
    dlg.SetItemInfo(1u, 2u);
    EXPECT_FALSE(dlg.SendShoutMsgSyn());
    EXPECT_EQ(dlg.GetItemIdx(), 1u);
    EXPECT_EQ(dlg.GetItemPos(), 2u);
}

}  // namespace mxh::ui::test
