#include "gtscoreinfodialog.hpp"

#include "mxh/ui/cButton.hpp"
#include "mxh/ui/cStatic.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>

using mxh::ui::cButton;
using mxh::ui::cDialog;
using mxh::ui::cGTScoreInfoDialog;
using mxh::ui::cStatic;
using mxh::ui::GTScoreBattleInfo;

namespace {

struct Controls {
    cStatic guildName1;
    cStatic guildName2;
    cStatic guildScore1;
    cStatic guildScore2;
    cStatic timeStatic;
    cButton outBtn;

    void Attach(cGTScoreInfoDialog& dialog) {
        dialog.SetControlsForTest(&guildName1, &guildName2,
                                  &guildScore1, &guildScore2,
                                  &timeStatic, &outBtn);
    }
};

struct TickState {
    std::uint32_t nextTick = 0;
};

TickState g_tickState;

std::uint32_t CapturedTick() {
    return g_tickState.nextTick;
}

struct TextCapture {
    std::string lastText;
    cStatic* lastControl = nullptr;
    int callCount = 0;
};

void CapturedTextWriter(cStatic* ctrl, const char* text, void* userData) {
    if (userData) {
        auto* cap = static_cast<TextCapture*>(userData);
        ++cap->callCount;
        cap->lastControl = ctrl;
        if (text) {
            cap->lastText = text;
        }
    }
    if (ctrl) {
        ctrl->SetStaticText(text ? text : "");
    }
}

GTScoreBattleInfo SampleInfo() {
    GTScoreBattleInfo info{};
    std::strncpy(info.guildName[0], "Blue", sizeof(info.guildName[0]) - 1);
    std::strncpy(info.guildName[1], "Red", sizeof(info.guildName[1]) - 1);
    info.score[0] = 7;
    info.score[1] = 12;
    info.entranceTime = 60000;
    info.fightTime = 90000;
    return info;
}

} // namespace

TEST(GTScoreInfoDialogTest, InheritsDialogAndIsNonCopyable) {
    static_assert(std::is_base_of_v<cDialog, cGTScoreInfoDialog>);
    static_assert(!std::is_copy_constructible_v<cGTScoreInfoDialog>);
    static_assert(!std::is_copy_assignable_v<cGTScoreInfoDialog>);
    SUCCEED();
}

TEST(GTScoreInfoDialogTest, ConstantsMatchPreprocessedLegacyWindowIds) {
    EXPECT_EQ(cGTScoreInfoDialog::kDialogId, 1326);
    EXPECT_EQ(cGTScoreInfoDialog::kGTimeId, 1394);
    EXPECT_EQ(cGTScoreInfoDialog::kOutBtnId, 1395);
    EXPECT_EQ(cGTScoreInfoDialog::kGuildName1Id, 1396);
    EXPECT_EQ(cGTScoreInfoDialog::kGuildMember1Id, 1397);
    EXPECT_EQ(cGTScoreInfoDialog::kGuildName2Id, 1398);
    EXPECT_EQ(cGTScoreInfoDialog::kGuildMember2Id, 1399);
    EXPECT_EQ(cGTScoreInfoDialog::kInitialEntranceMs, 120000u);
    EXPECT_EQ(cGTScoreInfoDialog::kMinuteMs, 60000u);
    EXPECT_EQ(cGTScoreInfoDialog::kSecondMs, 1000u);
    EXPECT_EQ(cGTScoreInfoDialog::kInvalidTeam, -1);
    EXPECT_EQ(cGTScoreInfoDialog::kTeamCount, 2u);
}

TEST(GTScoreInfoDialogTest, ConstructorDefaultsAreCorrect) {
    cGTScoreInfoDialog dialog;
    EXPECT_FALSE(dialog.battleStarted());
    EXPECT_EQ(dialog.fightTime(), 0u);
    EXPECT_EQ(dialog.entranceTime(), cGTScoreInfoDialog::kInitialEntranceMs);
    EXPECT_EQ(dialog.teamScore(0), 0);
    EXPECT_EQ(dialog.teamScore(1), 0);
    EXPECT_EQ(dialog.timeStaticControl(), nullptr);
    EXPECT_EQ(dialog.outButtonControl(), nullptr);
    EXPECT_EQ(dialog.guildNameControl(0), nullptr);
    EXPECT_EQ(dialog.guildScoreControl(1), nullptr);
    EXPECT_FALSE(dialog.isActive());
}

TEST(GTScoreInfoDialogTest, SetControlsForTestAssignsAllControls) {
    cGTScoreInfoDialog dialog;
    Controls controls;
    controls.Attach(dialog);

    EXPECT_EQ(dialog.guildNameControl(0), &controls.guildName1);
    EXPECT_EQ(dialog.guildNameControl(1), &controls.guildName2);
    EXPECT_EQ(dialog.guildScoreControl(0), &controls.guildScore1);
    EXPECT_EQ(dialog.guildScoreControl(1), &controls.guildScore2);
    EXPECT_EQ(dialog.timeStaticControl(), &controls.timeStatic);
    EXPECT_EQ(dialog.outButtonControl(), &controls.outBtn);
}

TEST(GTScoreInfoDialogTest, LinkingResolvesAllControlsAndResetsState) {
    cGTScoreInfoDialog dialog;
    dialog.Init(0, 0, 100, 100, nullptr, cGTScoreInfoDialog::kDialogId);
    dialog.StartBattle();
    dialog.SetTeamScore(0, 5);
    dialog.SetTeamScore(1, 9);

    auto addStatic = [&](std::int32_t id) {
        auto child = std::make_unique<cStatic>();
        child->Init(0, 0, 10, 10, nullptr, id);
        cStatic* raw = child.get();
        dialog.Add(std::move(child));
        return raw;
    };
    auto addButton = [&](std::int32_t id) {
        auto child = std::make_unique<cButton>();
        child->Init(0, 0, 10, 10, nullptr, nullptr, nullptr, {}, nullptr, id);
        cButton* raw = child.get();
        dialog.Add(std::move(child));
        return raw;
    };

    cStatic* name1 = addStatic(cGTScoreInfoDialog::kGuildName1Id);
    cStatic* name2 = addStatic(cGTScoreInfoDialog::kGuildName2Id);
    cStatic* score1 = addStatic(cGTScoreInfoDialog::kGuildMember1Id);
    cStatic* score2 = addStatic(cGTScoreInfoDialog::kGuildMember2Id);
    cStatic* timeS = addStatic(cGTScoreInfoDialog::kGTimeId);
    cButton* btn = addButton(cGTScoreInfoDialog::kOutBtnId);

    dialog.Linking();

    EXPECT_EQ(dialog.guildNameControl(0), name1);
    EXPECT_EQ(dialog.guildNameControl(1), name2);
    EXPECT_EQ(dialog.guildScoreControl(0), score1);
    EXPECT_EQ(dialog.guildScoreControl(1), score2);
    EXPECT_EQ(dialog.timeStaticControl(), timeS);
    EXPECT_EQ(dialog.outButtonControl(), btn);
    EXPECT_EQ(dialog.teamScore(0), 0);
    EXPECT_EQ(dialog.teamScore(1), 0);
    EXPECT_EQ(dialog.entranceTime(), cGTScoreInfoDialog::kInitialEntranceMs);
    EXPECT_FALSE(dialog.battleStarted());
}

TEST(GTScoreInfoDialogTest, SetBattleInfoUpdatesNamesScoresAndTimes) {
    cGTScoreInfoDialog dialog;
    Controls controls;
    controls.Attach(dialog);

    const auto info = SampleInfo();
    dialog.SetBattleInfo(info);

    EXPECT_EQ(controls.guildName1.GetStaticText(), "Blue");
    EXPECT_EQ(controls.guildName2.GetStaticText(), "Red");
    EXPECT_EQ(dialog.teamScore(0), 7);
    EXPECT_EQ(dialog.teamScore(1), 12);
    EXPECT_EQ(dialog.entranceTime(), 60000u);
    EXPECT_EQ(dialog.fightTime(), 90000u);
    EXPECT_EQ(controls.timeStatic.GetStaticText(), "01:30");
}

TEST(GTScoreInfoDialogTest, SetBattleInfoToleratesNullControls) {
    cGTScoreInfoDialog dialog;
    const auto info = SampleInfo();
    EXPECT_NO_FATAL_FAILURE(dialog.SetBattleInfo(info));
    EXPECT_EQ(dialog.teamScore(0), 7);
    EXPECT_EQ(dialog.entranceTime(), 60000u);
}

TEST(GTScoreInfoDialogTest, SetTeamScoreUpdatesCorrectSlot) {
    cGTScoreInfoDialog dialog;
    dialog.SetTeamScore(0, 42);
    dialog.SetTeamScore(1, -5);
    EXPECT_EQ(dialog.teamScore(0), 42);
    EXPECT_EQ(dialog.teamScore(1), -5);
}

TEST(GTScoreInfoDialogTest, SetTeamScoreIgnoresInvalidTeam) {
    cGTScoreInfoDialog dialog;
    dialog.SetTeamScore(0, 5);
    dialog.SetTeamScore(-1, 99);
    dialog.SetTeamScore(2, 99);
    dialog.SetTeamScore(99, 99);
    EXPECT_EQ(dialog.teamScore(0), 5);
    EXPECT_EQ(dialog.teamScore(1), 0);
}

TEST(GTScoreInfoDialogTest, StartBattleSetsFlag) {
    cGTScoreInfoDialog dialog;
    EXPECT_FALSE(dialog.battleStarted());
    dialog.StartBattle();
    EXPECT_TRUE(dialog.battleStarted());
}

TEST(GTScoreInfoDialogTest, EndBattleResetsFlagAndFightTime) {
    cGTScoreInfoDialog dialog;
    dialog.StartBattle();
    dialog.SetTeamScore(0, 3);
    dialog.SetBattleInfo(SampleInfo());
    EXPECT_EQ(dialog.fightTime(), 90000u);

    dialog.EndBattle();
    EXPECT_FALSE(dialog.battleStarted());
    EXPECT_EQ(dialog.fightTime(), 0u);
}

TEST(GTScoreInfoDialogTest, ShowOutBtnTogglesActive) {
    cGTScoreInfoDialog dialog;
    Controls controls;
    controls.Attach(dialog);

    EXPECT_FALSE(controls.outBtn.isActive());
    dialog.ShowOutBtn(true);
    EXPECT_TRUE(controls.outBtn.isActive());
    dialog.ShowOutBtn(false);
    EXPECT_FALSE(controls.outBtn.isActive());
}

TEST(GTScoreInfoDialogTest, ShowOutBtnToleratesNullButton) {
    cGTScoreInfoDialog dialog;
    EXPECT_NO_FATAL_FAILURE(dialog.ShowOutBtn(true));
    EXPECT_NO_FATAL_FAILURE(dialog.ShowOutBtn(false));
}

TEST(GTScoreInfoDialogTest, ProcessPreStartDeductsEntranceTime) {
    cGTScoreInfoDialog dialog;
    EXPECT_FALSE(dialog.battleStarted());
    EXPECT_EQ(dialog.entranceTime(), 120000u);

    g_tickState.nextTick = 5000;
    dialog.SetCallbacks(&CapturedTick, &CapturedTextWriter, nullptr);

    dialog.Process();
    EXPECT_EQ(dialog.entranceTime(), 115000u);
    dialog.Process();
    EXPECT_EQ(dialog.entranceTime(), 110000u);
}

TEST(GTScoreInfoDialogTest, ProcessPreStartClampsEntranceAtZero) {
    cGTScoreInfoDialog dialog;
    g_tickState.nextTick = 999999u;
    dialog.SetCallbacks(&CapturedTick, &CapturedTextWriter, nullptr);

    dialog.Process();
    EXPECT_EQ(dialog.entranceTime(), 0u);
    dialog.Process();
    EXPECT_EQ(dialog.entranceTime(), 0u);
    g_tickState.nextTick = 0;
}

TEST(GTScoreInfoDialogTest, ProcessPostStartDeductsFightTimeAndWritesText) {
    cGTScoreInfoDialog dialog;
    Controls controls;
    controls.Attach(dialog);

    const auto info = SampleInfo();
    dialog.SetBattleInfo(info);
    EXPECT_EQ(dialog.fightTime(), 90000u);
    EXPECT_EQ(controls.timeStatic.GetStaticText(), "01:30");

    dialog.StartBattle();
    g_tickState.nextTick = 30000;
    dialog.SetCallbacks(&CapturedTick, &CapturedTextWriter, nullptr);

    dialog.Process();
    EXPECT_EQ(dialog.fightTime(), 60000u);
    EXPECT_EQ(controls.timeStatic.GetStaticText(), "01:00");
    g_tickState.nextTick = 0;
}

TEST(GTScoreInfoDialogTest, ProcessPostStartClampsFightTimeAtZero) {
    cGTScoreInfoDialog dialog;
    Controls controls;
    controls.Attach(dialog);

    dialog.SetBattleInfo(SampleInfo());
    dialog.StartBattle();
    g_tickState.nextTick = 999999u;
    dialog.SetCallbacks(&CapturedTick, &CapturedTextWriter, nullptr);

    dialog.Process();
    EXPECT_EQ(dialog.fightTime(), 0u);
    EXPECT_EQ(controls.timeStatic.GetStaticText(), "00:00");
    g_tickState.nextTick = 0;
}

TEST(GTScoreInfoDialogTest, SetBattleInfoRoutesThroughCustomTextWriter) {
    cGTScoreInfoDialog dialog;
    Controls controls;
    controls.Attach(dialog);

    TextCapture capture;
    dialog.SetCallbacks(nullptr, &CapturedTextWriter, &capture);

    const auto info = SampleInfo();
    dialog.SetBattleInfo(info);

    EXPECT_EQ(capture.callCount, 3);
    EXPECT_EQ(capture.lastControl, &controls.timeStatic);
    EXPECT_EQ(capture.lastText, "01:30");
    EXPECT_EQ(controls.timeStatic.GetStaticText(), "01:30");
}

TEST(GTScoreInfoDialogTest, TeamScoreAccessorsReturnZeroForInvalidTeam) {
    cGTScoreInfoDialog dialog;
    dialog.SetTeamScore(0, 5);
    EXPECT_EQ(dialog.teamScore(-1), 0);
    EXPECT_EQ(dialog.teamScore(2), 0);
    EXPECT_EQ(dialog.teamScore(99), 0);
    EXPECT_EQ(dialog.teamScore(0), 5);
}

TEST(GTScoreInfoDialogTest, PollingAccessorsExposeAllControls) {
    cGTScoreInfoDialog dialog;
    Controls controls;
    controls.Attach(dialog);

    EXPECT_EQ(dialog.guildNameControl(0), &controls.guildName1);
    EXPECT_EQ(dialog.guildNameControl(1), &controls.guildName2);
    EXPECT_EQ(dialog.guildScoreControl(0), &controls.guildScore1);
    EXPECT_EQ(dialog.guildScoreControl(1), &controls.guildScore2);
    EXPECT_EQ(dialog.timeStaticControl(), &controls.timeStatic);
    EXPECT_EQ(dialog.outButtonControl(), &controls.outBtn);
}

TEST(GTScoreInfoDialogTest, ProcessToleratesZeroTick) {
    cGTScoreInfoDialog dialog;
    EXPECT_EQ(dialog.entranceTime(), 120000u);
    dialog.Process();
    EXPECT_EQ(dialog.entranceTime(), 120000u);

    dialog.StartBattle();
    dialog.SetBattleInfo(SampleInfo());
    const std::uint32_t before = dialog.fightTime();
    dialog.Process();
    EXPECT_EQ(dialog.fightTime(), before);
}
