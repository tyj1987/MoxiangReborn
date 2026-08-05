// Contract test for modern cMNPlayRoomDialog (MurimNet play-room dialog).
//
// Covers modern/src/ui/mnplayroomdialog.cpp/.hpp, a 1:1 port of
//   mnplayroomdialog.h (1,465 B) and
//   mnplayroomdialog.cpp (4,477 B).
//
// What is tested:
//   - Id constants (5 button ids + 3 list ids + chat edit/list + title)
//     match legacy WindowIDs.h rebase 730-740.
//   - DefaultConstructionHasNullPointers: all 9 child pointers +
//     no callbacks until Linking/callback injection.
//   - LinkingMaterializesAllChildren: 3 list + btn + edit + chat + title.
//   - LinkingSetsChildIds: each child gets its expected id.
//   - LinkingDisablesStartButtonByDefault: Start is inactive after Linking.
//   - OnActionEvent dispatches each button id to the matching callback:
//     MoveToA / MoveToB / Exit / Start fire their host callbacks;
//     MoveToOB is a documented no-op (legacy line is commented out).
//   - Callback storage / replacement / clearing via setters.
//   - Non-button events (e.g. WE_LBTNCLICK) are safe no-ops.
//   - Unknown button ids are safe no-ops.
//   - AddPlayer appends to the team roster + the linked cListDialog.
//   - AddPlayer with invalid team index is a safe no-op.
//   - RemovePlayer drops from roster + list (raw-name matching).
//   - RemoveAllPlayer clears all three rosters + lists.
//   - TeamChange moves a player from fromTeam to toTeam.
//   - SetCaptain toggles Start button active state + IsCaptain().
//   - SetPlayRoomInfo sets title text + stores full info struct.
//   - ChatMsg(PRCTC_WHOLE) formats [name]: msg and pushes to chat list.
//   - ChatMsg(PRCTC_TEAM/WHISPER) is silently dropped (legacy has
//     no switch case for them).
//   - PrintMsg adds raw string to chat list.
//   - ChatHistorySize + GetChatLine mirror the chat history.
//
// 1:1 quirks preserved from legacy MNPlayRoomDialog.cpp/h:
//   - MoveToOB button is recognized but fires no callback (legacy line
//     for SendMsgTeamChange(2) is commented out).
//   - m_pBtnStart is hidden (SetActive FALSE) at Linking time;
//     only SetCaptain(true) reveals it.
//   - ChatMsg switch only handles PRCTC_WHOLE.
//   - AddPlayer passes raw name (no level formatting).
//   - Team lists use raw-name RemoveItem (no formatting).

#include "mnplayroomdialog.hpp"
#include "cbutton.hpp"
#include "ceditbox.hpp"
#include "clistdialog.hpp"
#include "cstatic.hpp"
#include "legacy_window_event.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>

using mxh::ui::cMNPlayRoomDialog;
using mxh::ui::cButton;
using mxh::ui::cEditBox;
using mxh::ui::cListDialog;
using mxh::ui::cStatic;
using mxh::ui::PlayRoomTeam;
using mxh::ui::PlayRoomChatClass;
using mxh::ui::MNPlayerInfo;
using mxh::ui::MNPlayRoomInfo;

namespace {

std::unique_ptr<cMNPlayRoomDialog> MakeDialog() {
    auto d = std::make_unique<cMNPlayRoomDialog>();
    d->Init(0, 0, 400, 400, nullptr, 729);
    return d;
}

}  // namespace

// ===========================================================================
// Constants + ctor
// ===========================================================================

TEST(CMNPlayRoomDialogTest, NumTeamsMatchesLegacyEnum) {
    EXPECT_EQ(cMNPlayRoomDialog::kNumTeams, 3);
    EXPECT_EQ(static_cast<int>(PlayRoomTeam::TeamA),    0);
    EXPECT_EQ(static_cast<int>(PlayRoomTeam::TeamB),    1);
    EXPECT_EQ(static_cast<int>(PlayRoomTeam::Observer), 2);
    EXPECT_EQ(static_cast<int>(PlayRoomTeam::Max),      3);
}

TEST(CMNPlayRoomDialogTest, ButtonIdsInExpectedRange) {
    EXPECT_EQ(cMNPlayRoomDialog::kIdBtnMoveToA,  730);
    EXPECT_EQ(cMNPlayRoomDialog::kIdBtnMoveToB,  731);
    EXPECT_EQ(cMNPlayRoomDialog::kIdBtnMoveToOB, 732);
    EXPECT_EQ(cMNPlayRoomDialog::kIdBtnExit,     733);
    EXPECT_EQ(cMNPlayRoomDialog::kIdBtnStart,    734);
}

TEST(CMNPlayRoomDialogTest, ListAndChatIdsInExpectedRange) {
    EXPECT_EQ(cMNPlayRoomDialog::kIdListTeamA, 735);
    EXPECT_EQ(cMNPlayRoomDialog::kIdListTeamB, 736);
    EXPECT_EQ(cMNPlayRoomDialog::kIdListObs,   737);
    EXPECT_EQ(cMNPlayRoomDialog::kIdEdtChat,   738);
    EXPECT_EQ(cMNPlayRoomDialog::kIdListChat,  739);
    EXPECT_EQ(cMNPlayRoomDialog::kIdStcTitle,  740);
}

TEST(CMNPlayRoomDialogTest, AllIdsAreDistinct) {
    const int ids[] = {
        cMNPlayRoomDialog::kIdBtnMoveToA,
        cMNPlayRoomDialog::kIdBtnMoveToB,
        cMNPlayRoomDialog::kIdBtnMoveToOB,
        cMNPlayRoomDialog::kIdBtnExit,
        cMNPlayRoomDialog::kIdBtnStart,
        cMNPlayRoomDialog::kIdListTeamA,
        cMNPlayRoomDialog::kIdListTeamB,
        cMNPlayRoomDialog::kIdListObs,
        cMNPlayRoomDialog::kIdEdtChat,
        cMNPlayRoomDialog::kIdListChat,
        cMNPlayRoomDialog::kIdStcTitle,
    };
    for (std::size_t i = 0; i < sizeof(ids)/sizeof(ids[0]); ++i) {
        for (std::size_t j = i + 1; j < sizeof(ids)/sizeof(ids[0]); ++j) {
            EXPECT_NE(ids[i], ids[j]) << "duplicate ids at " << i << "," << j;
        }
    }
}

TEST(CMNPlayRoomDialogTest, DefaultConstructionHasNullPointers) {
    auto d = MakeDialog();
    EXPECT_EQ(d->GetListTeamA(), nullptr);
    EXPECT_EQ(d->GetListTeamB(), nullptr);
    EXPECT_EQ(d->GetListObs(),   nullptr);
    EXPECT_EQ(d->GetBtnStart(),  nullptr);
    EXPECT_EQ(d->GetChatEdit(),  nullptr);
    EXPECT_EQ(d->GetChatList(),  nullptr);
    EXPECT_EQ(d->GetTitle(),     nullptr);
    EXPECT_FALSE(d->HasCallbackSet());
    EXPECT_FALSE(d->IsCaptain());
    EXPECT_EQ(d->PlayerCount(PlayRoomTeam::TeamA), 0u);
    EXPECT_EQ(d->PlayerCount(PlayRoomTeam::TeamB), 0u);
    EXPECT_EQ(d->PlayerCount(PlayRoomTeam::Observer), 0u);
    EXPECT_EQ(d->ChatHistorySize(), 0u);
}

// ===========================================================================
// Linking
// ===========================================================================

TEST(CMNPlayRoomDialogTest, LinkingMaterializesAllChildren) {
    auto d = MakeDialog();
    d->Linking();
    EXPECT_NE(d->GetListTeamA(), nullptr);
    EXPECT_NE(d->GetListTeamB(), nullptr);
    EXPECT_NE(d->GetListObs(),   nullptr);
    EXPECT_NE(d->GetBtnStart(),  nullptr);
    EXPECT_NE(d->GetChatEdit(),  nullptr);
    EXPECT_NE(d->GetChatList(),  nullptr);
    EXPECT_NE(d->GetTitle(),     nullptr);
}

TEST(CMNPlayRoomDialogTest, LinkingSetsChildIds) {
    auto d = MakeDialog();
    d->Linking();
    EXPECT_EQ(d->GetListTeamA()->id(), 735);
    EXPECT_EQ(d->GetListTeamB()->id(), 736);
    EXPECT_EQ(d->GetListObs()->id(),   737);
    EXPECT_EQ(d->GetBtnStart()->id(),  734);
    EXPECT_EQ(d->GetChatEdit()->id(),  738);
    EXPECT_EQ(d->GetChatList()->id(),  739);
    EXPECT_EQ(d->GetTitle()->id(),     740);
}

TEST(CMNPlayRoomDialogTest, LinkingDisablesStartButtonByDefault) {
    // 1:1 quirk: legacy Linking sets m_pBtnStart->SetActive(FALSE).
    // Modern port mirrors it.
    auto d = MakeDialog();
    d->Linking();
    ASSERT_NE(d->GetBtnStart(), nullptr);
    EXPECT_FALSE(d->GetBtnStart()->isActive());
    EXPECT_FALSE(d->IsCaptain());  // captain flag mirrors button state
}

TEST(CMNPlayRoomDialogTest, LinkingIdempotent) {
    auto d = MakeDialog();
    d->Linking();
    d->Linking();
    EXPECT_NE(d->GetListTeamA(), nullptr);
    EXPECT_NE(d->GetListTeamB(), nullptr);
    EXPECT_NE(d->GetListObs(),   nullptr);
    EXPECT_NE(d->GetBtnStart(),  nullptr);
}

// ===========================================================================
// Callback injection
// ===========================================================================

TEST(CMNPlayRoomDialogTest, NoCallbacksByDefault) {
    auto d = MakeDialog();
    EXPECT_FALSE(d->HasCallbackSet());
    // Even with no callbacks, OnActionEvent must not crash.
    d->OnActionEvent(cMNPlayRoomDialog::kIdBtnExit,
                     nullptr, mxh::ui::legacy_window_event::kButtonClick);
}

TEST(CMNPlayRoomDialogTest, ClearCallbacksRemovesAll) {
    auto d = MakeDialog();
    d->SetTeamChangeCallback([](PlayRoomTeam){});
    d->SetExitRequestCallback([](){});
    d->SetStartRequestCallback([](){});
    d->SetChatSubmitCallback([](const std::string&){});
    EXPECT_TRUE(d->HasCallbackSet());
    d->ClearCallbacks();
    EXPECT_FALSE(d->HasCallbackSet());
}

// ===========================================================================
// OnActionEvent dispatch
// ===========================================================================

namespace {
// Test that all 5 button ids fire their matching callback host.
// Uses raw bool flags captured by lambdas; Lighter than storing full state.
struct CaptureState {
    int team_change_calls = 0;
    int exit_calls = 0;
    int start_calls = 0;
    int chat_submit_calls = 0;
    PlayRoomTeam last_team = PlayRoomTeam::Max;
    std::string last_chat;
};
}

TEST(CMNPlayRoomDialogTest, OnActionEventMoveToAFiresTeamChangeA) {
    auto d = MakeDialog();
    d->Linking();
    CaptureState cap;
    d->SetTeamChangeCallback([&](PlayRoomTeam t){
        ++cap.team_change_calls; cap.last_team = t;
    });
    d->OnActionEvent(cMNPlayRoomDialog::kIdBtnMoveToA, nullptr,
                     mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(cap.team_change_calls, 1);
    EXPECT_EQ(cap.last_team, PlayRoomTeam::TeamA);
    EXPECT_EQ(cap.exit_calls, 0);
}

TEST(CMNPlayRoomDialogTest, OnActionEventMoveToBFiresTeamChangeB) {
    auto d = MakeDialog();
    d->Linking();
    CaptureState cap;
    d->SetTeamChangeCallback([&](PlayRoomTeam t){
        ++cap.team_change_calls; cap.last_team = t;
    });
    d->OnActionEvent(cMNPlayRoomDialog::kIdBtnMoveToB, nullptr,
                     mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(cap.team_change_calls, 1);
    EXPECT_EQ(cap.last_team, PlayRoomTeam::TeamB);
}

TEST(CMNPlayRoomDialogTest, OnActionEventMoveToOBIsNoOp) {
    // 1:1 quirk: legacy line is commented out (no SendMsgTeamChange(2)).
    auto d = MakeDialog();
    d->Linking();
    CaptureState cap;
    d->SetTeamChangeCallback([&](PlayRoomTeam t){
        ++cap.team_change_calls; cap.last_team = t;
    });
    d->OnActionEvent(cMNPlayRoomDialog::kIdBtnMoveToOB, nullptr,
                     mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(cap.team_change_calls, 0);
    EXPECT_EQ(cap.last_team, PlayRoomTeam::Max);
}

TEST(CMNPlayRoomDialogTest, OnActionEventExitFiresExitCallback) {
    auto d = MakeDialog();
    d->Linking();
    CaptureState cap;
    d->SetExitRequestCallback([&](){ ++cap.exit_calls; });
    d->SetStartRequestCallback([&](){ ++cap.start_calls; });
    d->OnActionEvent(cMNPlayRoomDialog::kIdBtnExit, nullptr,
                     mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(cap.exit_calls, 1);
    EXPECT_EQ(cap.start_calls, 0);
}

TEST(CMNPlayRoomDialogTest, OnActionEventStartFiresStartCallback) {
    auto d = MakeDialog();
    d->Linking();
    CaptureState cap;
    d->SetExitRequestCallback([&](){ ++cap.exit_calls; });
    d->SetStartRequestCallback([&](){ ++cap.start_calls; });
    d->OnActionEvent(cMNPlayRoomDialog::kIdBtnStart, nullptr,
                     mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(cap.start_calls, 1);
    EXPECT_EQ(cap.exit_calls, 0);
}

TEST(CMNPlayRoomDialogTest, OnActionEventUnknownIdIsNoOp) {
    auto d = MakeDialog();
    d->Linking();
    int total = 0;
    d->SetTeamChangeCallback([&](PlayRoomTeam){ ++total; });
    d->SetExitRequestCallback([&](){ ++total; });
    d->SetStartRequestCallback([&](){ ++total; });
    d->OnActionEvent(99999, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(total, 0);
}

TEST(CMNPlayRoomDialogTest, OnActionEventNonButtonEventIsNoOp) {
    // 1:1 quirk: legacy guards if (we == WE_BTNCLICK); modern port
    // uses bitmask against kButtonClick.
    auto d = MakeDialog();
    d->Linking();
    int total = 0;
    d->SetTeamChangeCallback([&](PlayRoomTeam){ ++total; });
    d->SetExitRequestCallback([&](){ ++total; });
    d->SetStartRequestCallback([&](){ ++total; });
    d->OnActionEvent(cMNPlayRoomDialog::kIdBtnStart, nullptr,
                     42 /* some other event flag */);
    EXPECT_EQ(total, 0);
}

TEST(CMNPlayRoomDialogTest, OnActionEventWithoutLinkingIsSafe) {
    // Guards against use-before-Link: no children, no callbacks.
    auto d = MakeDialog();
    d->OnActionEvent(cMNPlayRoomDialog::kIdBtnExit, nullptr,
                     mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(d->GetBtnStart(), nullptr);
}

// ===========================================================================
// AddPlayer / RemovePlayer / RemoveAllPlayer / TeamChange
// ===========================================================================

namespace {
MNPlayerInfo MakePlayer(const std::string& name, PlayRoomTeam team) {
    MNPlayerInfo p;
    p.name = name;
    p.level = 1;
    p.team = team;
    return p;
}
}

TEST(CMNPlayRoomDialogTest, AddPlayerAppendsToCorrectTeam) {
    auto d = MakeDialog();
    d->Linking();
    d->AddPlayer(MakePlayer("Alice", PlayRoomTeam::TeamA));
    d->AddPlayer(MakePlayer("Bob",   PlayRoomTeam::TeamB));
    d->AddPlayer(MakePlayer("Cara",  PlayRoomTeam::Observer));
    EXPECT_EQ(d->PlayerCount(PlayRoomTeam::TeamA), 1u);
    EXPECT_EQ(d->PlayerCount(PlayRoomTeam::TeamB), 1u);
    EXPECT_EQ(d->PlayerCount(PlayRoomTeam::Observer), 1u);
    EXPECT_EQ(d->PlayerAt(PlayRoomTeam::TeamA, 0), "Alice");
    EXPECT_EQ(d->PlayerAt(PlayRoomTeam::TeamB, 0), "Bob");
    EXPECT_EQ(d->PlayerAt(PlayRoomTeam::Observer, 0), "Cara");
    // And the linked cListDialog row counts mirror.
    EXPECT_EQ(d->GetListTeamA()->RowCount(), 1u);
    EXPECT_EQ(d->GetListTeamB()->RowCount(), 1u);
    EXPECT_EQ(d->GetListObs()->RowCount(),   1u);
}

TEST(CMNPlayRoomDialogTest, AddPlayerInvalidTeamIsNoOp) {
    // 1:1 with modern port: AddPlayer(PlayRoomTeam::Max) is safe no-op.
    auto d = MakeDialog();
    d->Linking();
    d->AddPlayer(MakePlayer("X", PlayRoomTeam::Max));
    EXPECT_EQ(d->PlayerCount(PlayRoomTeam::TeamA), 0u);
    EXPECT_EQ(d->PlayerCount(PlayRoomTeam::TeamB), 0u);
    EXPECT_EQ(d->PlayerCount(PlayRoomTeam::Observer), 0u);
}

TEST(CMNPlayRoomDialogTest, RemovePlayerRemovesFromTeam) {
    auto d = MakeDialog();
    d->Linking();
    d->AddPlayer(MakePlayer("Alice", PlayRoomTeam::TeamA));
    d->AddPlayer(MakePlayer("Bob",   PlayRoomTeam::TeamA));
    d->RemovePlayer("Bob", PlayRoomTeam::TeamA);
    EXPECT_EQ(d->PlayerCount(PlayRoomTeam::TeamA), 1u);
    EXPECT_EQ(d->PlayerAt(PlayRoomTeam::TeamA, 0), "Alice");
    EXPECT_EQ(d->GetListTeamA()->RowCount(), 1u);
}

TEST(CMNPlayRoomDialogTest, RemovePlayerNotFoundIsNoOp) {
    auto d = MakeDialog();
    d->Linking();
    d->AddPlayer(MakePlayer("Alice", PlayRoomTeam::TeamA));
    d->RemovePlayer("Nobody", PlayRoomTeam::TeamA);
    EXPECT_EQ(d->PlayerCount(PlayRoomTeam::TeamA), 1u);
}

TEST(CMNPlayRoomDialogTest, RemoveAllPlayerClearsAllTeams) {
    auto d = MakeDialog();
    d->Linking();
    d->AddPlayer(MakePlayer("A", PlayRoomTeam::TeamA));
    d->AddPlayer(MakePlayer("B", PlayRoomTeam::TeamB));
    d->AddPlayer(MakePlayer("C", PlayRoomTeam::Observer));
    d->RemoveAllPlayer();
    EXPECT_EQ(d->PlayerCount(PlayRoomTeam::TeamA),    0u);
    EXPECT_EQ(d->PlayerCount(PlayRoomTeam::TeamB),    0u);
    EXPECT_EQ(d->PlayerCount(PlayRoomTeam::Observer), 0u);
    EXPECT_EQ(d->GetListTeamA()->RowCount(), 0u);
    EXPECT_EQ(d->GetListTeamB()->RowCount(), 0u);
    EXPECT_EQ(d->GetListObs()->RowCount(),   0u);
}

TEST(CMNPlayRoomDialogTest, TeamChangeMovesPlayer) {
    auto d = MakeDialog();
    d->Linking();
    d->AddPlayer(MakePlayer("Alice", PlayRoomTeam::TeamA));
    d->TeamChange("Alice", PlayRoomTeam::TeamA, PlayRoomTeam::TeamB);
    EXPECT_EQ(d->PlayerCount(PlayRoomTeam::TeamA), 0u);
    EXPECT_EQ(d->PlayerCount(PlayRoomTeam::TeamB), 1u);
    EXPECT_EQ(d->PlayerAt(PlayRoomTeam::TeamB, 0), "Alice");
}

TEST(CMNPlayRoomDialogTest, TeamChangeSameTeamIsNoOp) {
    auto d = MakeDialog();
    d->Linking();
    d->AddPlayer(MakePlayer("Alice", PlayRoomTeam::TeamA));
    d->TeamChange("Alice", PlayRoomTeam::TeamA, PlayRoomTeam::TeamA);
    // Player is removed then re-added in-place; count unchanged.
    EXPECT_EQ(d->PlayerCount(PlayRoomTeam::TeamA), 1u);
    EXPECT_EQ(d->PlayerAt(PlayRoomTeam::TeamA, 0), "Alice");
}

// ===========================================================================
// SetCaptain
// ===========================================================================

TEST(CMNPlayRoomDialogTest, SetCaptainTrueActivatesStartButton) {
    auto d = MakeDialog();
    d->Linking();
    ASSERT_FALSE(d->IsCaptain());
    ASSERT_FALSE(d->GetBtnStart()->isActive());
    d->SetCaptain(true);
    EXPECT_TRUE(d->IsCaptain());
    EXPECT_TRUE(d->GetBtnStart()->isActive());
}

TEST(CMNPlayRoomDialogTest, SetCaptainFalseDeactivatesStartButton) {
    auto d = MakeDialog();
    d->Linking();
    d->SetCaptain(true);
    d->SetCaptain(false);
    EXPECT_FALSE(d->IsCaptain());
    EXPECT_FALSE(d->GetBtnStart()->isActive());
}

TEST(CMNPlayRoomDialogTest, SetCaptainBeforeLinkingIsSafe) {
    // Before Linking, m_pBtnStart is nullptr; SetCaptain just flips the flag.
    auto d = MakeDialog();
    d->SetCaptain(true);
    EXPECT_TRUE(d->IsCaptain());
    // Linking materializes the button but captain is still true.
    d->Linking();
    EXPECT_TRUE(d->IsCaptain());  // 1:1 with legacy behavior
    EXPECT_TRUE(d->GetBtnStart()->isActive());
}

// ===========================================================================
// SetPlayRoomInfo
// ===========================================================================

TEST(CMNPlayRoomDialogTest, SetPlayRoomInfoUpdatesTitle) {
    auto d = MakeDialog();
    d->Linking();
    MNPlayRoomInfo info;
    info.title = "PVP Arena 7";
    info.max_players = 8;
    info.game_kind = 1;
    info.room_kind = 2;
    info.repay = 100;
    d->SetPlayRoomInfo(info);
    EXPECT_EQ(d->GetTitle()->GetStaticText(), "PVP Arena 7");
    auto stored = d->GetPlayRoomInfo();
    EXPECT_EQ(stored.title, "PVP Arena 7");
    EXPECT_EQ(stored.max_players, 8u);
    EXPECT_EQ(stored.game_kind, 1u);
    EXPECT_EQ(stored.room_kind, 2u);
    EXPECT_EQ(stored.repay, 100u);
}

TEST(CMNPlayRoomDialogTest, SetPlayRoomInfoDefensiveBeforeLinking) {
    auto d = MakeDialog();
    MNPlayRoomInfo info;
    info.title = "X";
    d->SetPlayRoomInfo(info);  // m_pTitle is nullptr; no crash
    EXPECT_EQ(d->GetTitle(), nullptr);
    EXPECT_EQ(d->GetPlayRoomInfo().title, "X");
}

TEST(CMNPlayRoomDialogTest, SetPlayRoomInfoEmptyTitleKeepsDefault) {
    auto d = MakeDialog();
    d->Linking();
    MNPlayRoomInfo info;  // title is empty
    d->SetPlayRoomInfo(info);
    // Empty title: we do NOT call SetStaticText.
    // Default cStatic text remains untouched.
    EXPECT_EQ(d->GetPlayRoomInfo().title, std::string());
}

// ===========================================================================
// ChatMsg / PrintMsg
// ===========================================================================

TEST(CMNPlayRoomDialogTest, ChatMsgWholeFormatsNameAndPushesChatLine) {
    // 1:1 with legacy ChatMsg(PRCTC_WHOLE): formats [name]: msg.
    auto d = MakeDialog();
    d->Linking();
    d->ChatMsg(PlayRoomChatClass::Whole, "Alice", "hello");
    ASSERT_EQ(d->ChatHistorySize(), 1u);
    auto line = d->GetChatLine(0);
    EXPECT_NE(line.find("[Alice]"), std::string::npos);
    EXPECT_NE(line.find("hello"), std::string::npos);
    EXPECT_EQ(d->GetChatList()->RowCount(), 1u);
}

TEST(CMNPlayRoomDialogTest, ChatMsgTeamAndWhisperAreSilentlyDropped) {
    // 1:1 quirk: legacy ChatMsg switch has no case for PRCTC_TEAM /
    // PRCTC_WHISPER, so those branches are absent; modern port follows.
    auto d = MakeDialog();
    d->Linking();
    d->ChatMsg(PlayRoomChatClass::Team, "A", "B");
    d->ChatMsg(PlayRoomChatClass::Whisper, "C", "D");
    EXPECT_EQ(d->ChatHistorySize(), 0u);
    EXPECT_EQ(d->GetChatList()->RowCount(), 0u);
}

TEST(CMNPlayRoomDialogTest, PrintMsgAddsRawString) {
    // 1:1 with legacy PrintMsg: adds raw msg to chat list, no format.
    auto d = MakeDialog();
    d->Linking();
    d->PrintMsg(PlayRoomChatClass::Team, "system notice");
    ASSERT_EQ(d->ChatHistorySize(), 1u);
    EXPECT_EQ(d->GetChatLine(0), "system notice");
}

TEST(CMNPlayRoomDialogTest, ChatMixedSourcesPreserveOrder) {
    auto d = MakeDialog();
    d->Linking();
    d->ChatMsg(PlayRoomChatClass::Whole, "Alice", "hi");
    d->PrintMsg(PlayRoomChatClass::Whole, "system: ready");
    d->ChatMsg(PlayRoomChatClass::Whole, "Bob", "yo");
    ASSERT_EQ(d->ChatHistorySize(), 3u);
    EXPECT_NE(d->GetChatLine(0).find("Alice"), std::string::npos);
    EXPECT_EQ(d->GetChatLine(1), "system: ready");
    EXPECT_NE(d->GetChatLine(2).find("Bob"), std::string::npos);
}

TEST(CMNPlayRoomDialogTest, GetChatLineOutOfRangeThrows) {
    // cMNPlayRoomDialog::GetChatLine mirrors std::vector::at() bounds.
    auto d = MakeDialog();
    d->Linking();
    EXPECT_THROW(d->GetChatLine(0), std::out_of_range);
    d->PrintMsg(PlayRoomChatClass::Whole, "a");
    EXPECT_NO_THROW(d->GetChatLine(0));
    EXPECT_THROW(d->GetChatLine(1), std::out_of_range);
}

TEST(CMNPlayRoomDialogTest, PlayerAtOutOfRangeReturnsEmpty) {
    // 1:1 quirk: defensive returns empty string for invalid index.
    auto d = MakeDialog();
    d->Linking();
    EXPECT_EQ(d->PlayerAt(PlayRoomTeam::TeamA, 0), std::string());
    EXPECT_EQ(d->PlayerAt(PlayRoomTeam::Max, 0), std::string());
}

