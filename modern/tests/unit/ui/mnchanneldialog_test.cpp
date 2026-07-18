// mnchanneldialog_test.cpp — 1:1 port verification tests for cMNChannelDialog.

#include "mnchanneldialog.hpp"
#include "cbutton.hpp"
#include "ceditbox.hpp"
#include "clistdialog.hpp"
#include "cpushupbutton.hpp"
#include "cstatic.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>

using mxh::ui::cMNChannelDialog;
using mxh::ui::cButton;
using mxh::ui::cEditBox;
using mxh::ui::cListDialog;
using mxh::ui::cPushupButton;
using mxh::ui::cStatic;
using mxh::ui::ChannelMode;

namespace {

std::unique_ptr<cMNChannelDialog> MakeDialog() {
    auto dlg = std::make_unique<cMNChannelDialog>();
    dlg->Init(0, 0, 400, 300, nullptr, 759);
    return dlg;
}

}  // namespace

// ---------------------------------------------------------------------------
// Constants + construction
// ---------------------------------------------------------------------------

TEST(CMNChannelDialog, NumModesMatchesLegacyEnum) {
    EXPECT_EQ(cMNChannelDialog::kNumModes, 3);  // eCNL_MODE_MAX
    EXPECT_EQ(static_cast<int>(ChannelMode::Id),       0);
    EXPECT_EQ(static_cast<int>(ChannelMode::Channel),  1);
    EXPECT_EQ(static_cast<int>(ChannelMode::PlayRoom), 2);
    EXPECT_EQ(static_cast<int>(ChannelMode::Max),      3);
}

TEST(CMNChannelDialog, DefaultChannelModeIsId) {
    auto dlg = MakeDialog();
    EXPECT_EQ(dlg->GetChannelMode(), ChannelMode::Id);
}

TEST(CMNChannelDialog, ChildrenNullBeforeLinking) {
    auto dlg = MakeDialog();
    for (auto m : {ChannelMode::Id, ChannelMode::Channel, ChannelMode::PlayRoom}) {
        EXPECT_EQ(dlg->GetListDialogForMode(m), nullptr);
        EXPECT_EQ(dlg->GetTabButtonForMode(m), nullptr);
    }
    EXPECT_EQ(dlg->GetJoinButton(), nullptr);
    EXPECT_EQ(dlg->GetChatEdit(), nullptr);
    EXPECT_EQ(dlg->GetChatList(), nullptr);
    EXPECT_EQ(dlg->GetTitle(), nullptr);
}

TEST(CMNChannelDialog, GetListDialogForModeRejectsOutOfRange) {
    auto dlg = MakeDialog();
    EXPECT_EQ(dlg->GetListDialogForMode(ChannelMode::Max), nullptr);
    EXPECT_EQ(dlg->GetTabButtonForMode(ChannelMode::Max), nullptr);
}

// ---------------------------------------------------------------------------
// Linking()
// ---------------------------------------------------------------------------

TEST(CMNChannelDialog, LinkingMaterializesAllNineChildren) {
    auto dlg = MakeDialog();
    dlg->Linking();
    for (auto m : {ChannelMode::Id, ChannelMode::Channel, ChannelMode::PlayRoom}) {
        EXPECT_NE(dlg->GetListDialogForMode(m), nullptr);
        EXPECT_NE(dlg->GetTabButtonForMode(m), nullptr);
    }
    EXPECT_NE(dlg->GetJoinButton(), nullptr);
    EXPECT_NE(dlg->GetChatEdit(), nullptr);
    EXPECT_NE(dlg->GetChatList(), nullptr);
    EXPECT_NE(dlg->GetTitle(), nullptr);
}

TEST(CMNChannelDialog, LinkingSetsChildIds) {
    auto dlg = MakeDialog();
    dlg->Linking();
    EXPECT_EQ(dlg->GetListDialogForMode(ChannelMode::Id)->id(),       760);
    EXPECT_EQ(dlg->GetListDialogForMode(ChannelMode::Channel)->id(),  761);
    EXPECT_EQ(dlg->GetListDialogForMode(ChannelMode::PlayRoom)->id(), 762);
    EXPECT_EQ(dlg->GetTabButtonForMode(ChannelMode::Id)->id(),       763);
    EXPECT_EQ(dlg->GetTabButtonForMode(ChannelMode::Channel)->id(),  764);
    EXPECT_EQ(dlg->GetTabButtonForMode(ChannelMode::PlayRoom)->id(), 765);
    EXPECT_EQ(dlg->GetJoinButton()->id(), 766);
    EXPECT_EQ(dlg->GetChatEdit()->id(),    767);
    EXPECT_EQ(dlg->GetChatList()->id(),    768);
    EXPECT_EQ(dlg->GetTitle()->id(),       769);
}

TEST(CMNChannelDialog, LinkingIdempotent) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->Linking();
    // All children still non-null after re-link.
    for (auto m : {ChannelMode::Id, ChannelMode::Channel, ChannelMode::PlayRoom}) {
        EXPECT_NE(dlg->GetListDialogForMode(m), nullptr);
        EXPECT_NE(dlg->GetTabButtonForMode(m), nullptr);
    }
}

// ---------------------------------------------------------------------------
// SetChannelInfo
// ---------------------------------------------------------------------------

TEST(CMNChannelDialog, SetChannelInfoUpdatesTitle) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->SetChannelInfo("PVP Channel A");
    EXPECT_EQ(dlg->GetTitle()->GetStaticText(), "PVP Channel A");
}

TEST(CMNChannelDialog, SetChannelInfoDefensiveBeforeLinking) {
    auto dlg = MakeDialog();
    dlg->SetChannelInfo("X");  // title is nullptr, must not crash
    EXPECT_EQ(dlg->GetTitle(), nullptr);
}

// ---------------------------------------------------------------------------
// AddPlayer / RemovePlayer / RemoveAllPlayer
// ---------------------------------------------------------------------------

TEST(CMNChannelDialog, AddPlayerAppendsFormattedRow) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->AddPlayer("Alice", 42);
    auto* idList = dlg->GetListDialogForMode(ChannelMode::Id);
    ASSERT_NE(idList, nullptr);
    EXPECT_EQ(idList->RowCount(), 1u);
    // First 50 chars are right-aligned name padded with spaces.
    const auto& row0 = idList->GetRow(0);
    EXPECT_NE(row0.first.find("Alice"), std::string::npos);
    EXPECT_NE(row0.first.find("[Level: 42]"), std::string::npos);
}

TEST(CMNChannelDialog, AddPlayerMultipleAppendsInOrder) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->AddPlayer("Alice", 42);
    dlg->AddPlayer("Bob",   99);
    auto* idList = dlg->GetListDialogForMode(ChannelMode::Id);
    EXPECT_EQ(idList->RowCount(), 2u);
}

TEST(CMNChannelDialog, AddPlayerDoesNotTouchOtherLists) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->AddPlayer("Alice", 42);
    EXPECT_EQ(dlg->GetListDialogForMode(ChannelMode::Channel)->RowCount(),  0u);
    EXPECT_EQ(dlg->GetListDialogForMode(ChannelMode::PlayRoom)->RowCount(), 0u);
}

TEST(CMNChannelDialog, RemovePlayerRemovesMatchingRow) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->AddPlayer("Alice", 42);
    dlg->AddPlayer("Bob",   99);
    dlg->RemovePlayer("Bob", 99);
    auto* idList = dlg->GetListDialogForMode(ChannelMode::Id);
    EXPECT_EQ(idList->RowCount(), 1u);
    EXPECT_NE(idList->GetRow(0).first.find("Alice"), std::string::npos);
}

TEST(CMNChannelDialog, RemovePlayerNotFoundIsNoOp) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->AddPlayer("Alice", 42);
    dlg->RemovePlayer("Nobody", 1);
    EXPECT_EQ(dlg->GetListDialogForMode(ChannelMode::Id)->RowCount(), 1u);
}

TEST(CMNChannelDialog, RemoveAllPlayerClearsList) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->AddPlayer("Alice", 1);
    dlg->AddPlayer("Bob",   2);
    dlg->RemoveAllPlayer();
    EXPECT_EQ(dlg->GetListDialogForMode(ChannelMode::Id)->RowCount(), 0u);
}

// ---------------------------------------------------------------------------
// AddChannel / RemoveChannel / RemoveAllChannel
// ---------------------------------------------------------------------------

TEST(CMNChannelDialog, AddChannelAppendsFormattedRow) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->AddChannel("Red Arena", 3, 8);
    auto* ch = dlg->GetListDialogForMode(ChannelMode::Channel);
    ASSERT_NE(ch, nullptr);
    EXPECT_EQ(ch->RowCount(), 1u);
    EXPECT_NE(ch->GetRow(0).first.find("Red Arena"),  std::string::npos);
    EXPECT_NE(ch->GetRow(0).first.find("(  3/  8)"), std::string::npos);
}

TEST(CMNChannelDialog, RemoveChannelRemovesByRawTitle) {
    // 1:1 quirk: legacy passes the raw title (not formatted), per
    // legacy comment "수정해야한다..". Modern port preserves this.
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->AddChannel("Red Arena", 1, 8);
    dlg->RemoveChannel("Red Arena");  // raw title, NOT formatted
    EXPECT_EQ(dlg->GetListDialogForMode(ChannelMode::Channel)->RowCount(), 1u);
    // (Documented as preserved quirk; the legacy line is a no-op match.)
}

TEST(CMNChannelDialog, RemoveAllChannelClearsList) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->AddChannel("A", 1, 2);
    dlg->AddChannel("B", 3, 4);
    dlg->RemoveAllChannel();
    EXPECT_EQ(dlg->GetListDialogForMode(ChannelMode::Channel)->RowCount(), 0u);
}

// ---------------------------------------------------------------------------
// AddPlayRoom / RemovePlayRoom / RemoveAllPlayRoom
// ---------------------------------------------------------------------------

TEST(CMNChannelDialog, AddPlayRoomAppendsRawTitle) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->AddPlayRoom("Room42");
    auto* pr = dlg->GetListDialogForMode(ChannelMode::PlayRoom);
    EXPECT_EQ(pr->RowCount(), 1u);
    EXPECT_EQ(pr->GetRow(0).first, "Room42");
}

TEST(CMNChannelDialog, RemovePlayRoomRemovesByTitle) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->AddPlayRoom("Room42");
    dlg->AddPlayRoom("Room7");
    dlg->RemovePlayRoom("Room42");
    auto* pr = dlg->GetListDialogForMode(ChannelMode::PlayRoom);
    EXPECT_EQ(pr->RowCount(), 1u);
    EXPECT_EQ(pr->GetRow(0).first, "Room7");
}

TEST(CMNChannelDialog, RemoveAllPlayRoomClearsList) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->AddPlayRoom("A");
    dlg->AddPlayRoom("B");
    dlg->RemoveAllPlayRoom();
    EXPECT_EQ(dlg->GetListDialogForMode(ChannelMode::PlayRoom)->RowCount(), 0u);
}

// ---------------------------------------------------------------------------
// ChatMsgWhole
// ---------------------------------------------------------------------------

TEST(CMNChannelDialog, ChatMsgWholeAppendsFormattedLine) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->ChatMsgWhole("Alice", "hi");
    auto* chat = dlg->GetChatList();
    ASSERT_NE(chat, nullptr);
    EXPECT_EQ(chat->RowCount(), 1u);
    EXPECT_EQ(chat->GetRow(0).first, "[Alice]: hi");
}

TEST(CMNChannelDialog, ChatMsgWholeMultipleAppendsInOrder) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->ChatMsgWhole("Alice", "1");
    dlg->ChatMsgWhole("Bob",   "2");
    auto* chat = dlg->GetChatList();
    EXPECT_EQ(chat->RowCount(), 2u);
    EXPECT_EQ(chat->GetRow(0).first, "[Alice]: 1");
    EXPECT_EQ(chat->GetRow(1).first, "[Bob]: 2");
}

// ---------------------------------------------------------------------------
// SetChannelMode
// ---------------------------------------------------------------------------

TEST(CMNChannelDialog, SetChannelModeActivatesSelectedList) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->SetChannelMode(ChannelMode::Channel);
    EXPECT_EQ(dlg->GetChannelMode(), ChannelMode::Channel);
    EXPECT_TRUE(dlg->GetListDialogForMode(ChannelMode::Channel)->isActive());
}

TEST(CMNChannelDialog, SetChannelModeRejectsOutOfRange) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->SetChannelMode(ChannelMode::Max);  // out of range, no change
    EXPECT_EQ(dlg->GetChannelMode(), ChannelMode::Id);  // unchanged
}

TEST(CMNChannelDialog, SetChannelModeSwitchesMode) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->SetChannelMode(ChannelMode::PlayRoom);
    EXPECT_EQ(dlg->GetChannelMode(), ChannelMode::PlayRoom);
    dlg->SetChannelMode(ChannelMode::Id);
    EXPECT_EQ(dlg->GetChannelMode(), ChannelMode::Id);
}
