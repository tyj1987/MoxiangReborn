#include "guildrankdialog.hpp"

#include "mxh/ui/cButton.hpp"
#include "mxh/ui/cComboBox.hpp"
#include "mxh/ui/cStatic.hpp"
#include "mxh/ui/cTextArea.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>

using mxh::ui::cButton;
using mxh::ui::cComboBox;
using mxh::ui::cDialog;
using mxh::ui::cGuildRankDialog;
using mxh::ui::cStatic;
using mxh::ui::cTextArea;
using mxh::ui::GuildRankMode;
using mxh::ui::GuildRankSelection;

namespace {

struct Controls {
    cTextArea memberName;
    cComboBox guildRank;
    cComboBox danRank;
    cButton guildOk;
    cButton danOk;

    void Attach(cGuildRankDialog& dialog) {
        dialog.SetControlsForTest(&memberName, &guildRank, &danRank,
                                  &guildOk, &danOk);
    }
};

struct ChatCapture {
    int textCalls = 0;
    int systemCalls = 0;
    int lastMessageId = 0;
    std::string systemMessage;
};

const char* ChatText(std::int32_t messageId, void* userData) {
    auto* capture = static_cast<ChatCapture*>(userData);
    ++capture->textCalls;
    capture->lastMessageId = messageId;
    return messageId == cGuildRankDialog::kMemberNameFormatMessageId
        ? "Member:%s"
        : "Invalid member";
}

void SystemMessage(const char* message, void* userData) {
    auto* capture = static_cast<ChatCapture*>(userData);
    ++capture->systemCalls;
    capture->systemMessage = message ? message : "";
}

GuildRankSelection ValidSelection() {
    return GuildRankSelection{10, 99, "Alice"};
}

} // namespace

TEST(GuildRankDialogTest, InheritsDialogAndIsNonCopyable) {
    static_assert(std::is_base_of_v<cDialog, cGuildRankDialog>);
    static_assert(!std::is_copy_constructible_v<cGuildRankDialog>);
    static_assert(!std::is_copy_assignable_v<cGuildRankDialog>);
    SUCCEED();
}

TEST(GuildRankDialogTest, ConstantsMatchPreprocessedLegacyValues) {
    EXPECT_EQ(static_cast<int>(GuildRankMode::Dan), 0);
    EXPECT_EQ(static_cast<int>(GuildRankMode::Guild), 1);
    EXPECT_EQ(static_cast<int>(GuildRankMode::Max), 2);
    EXPECT_EQ(cGuildRankDialog::kUnsetMode, 255);
    EXPECT_EQ(cGuildRankDialog::kMaxGuildLevel, 5);
    EXPECT_EQ(cGuildRankDialog::kDialogId, 546);
    EXPECT_EQ(cGuildRankDialog::kDanRankComboId, 547);
    EXPECT_EQ(cGuildRankDialog::kGuildRankComboId, 548);
    EXPECT_EQ(cGuildRankDialog::kDanOkButtonId, 549);
    EXPECT_EQ(cGuildRankDialog::kGuildOkButtonId, 550);
    EXPECT_EQ(cGuildRankDialog::kMemberNameId, 551);
}

TEST(GuildRankDialogTest, ConstructorMatchesLegacyDefaults) {
    cGuildRankDialog dialog;
    EXPECT_EQ(dialog.currentMode(), cGuildRankDialog::kUnsetMode);
    EXPECT_EQ(dialog.memberNameControl(), nullptr);
    EXPECT_EQ(dialog.guildRankCombo(), nullptr);
    EXPECT_EQ(dialog.danRankCombo(), nullptr);
    EXPECT_EQ(dialog.guildOkButton(), nullptr);
    EXPECT_EQ(dialog.danOkButton(), nullptr);
    EXPECT_FALSE(dialog.isActive());
}

TEST(GuildRankDialogTest, LinkingResolvesAllFiveControls) {
    cGuildRankDialog dialog;
    auto memberName = std::make_unique<cTextArea>();
    auto danRank = std::make_unique<cComboBox>();
    auto guildRank = std::make_unique<cComboBox>();
    auto danOk = std::make_unique<cButton>();
    auto guildOk = std::make_unique<cButton>();
    memberName->cWindow::Init(0, 0, 20, 10, nullptr, 551);
    danRank->cWindow::Init(0, 0, 20, 10, nullptr, 547);
    guildRank->cWindow::Init(0, 0, 20, 10, nullptr, 548);
    danOk->Init(0, 0, 20, 10, nullptr, nullptr, nullptr, {}, nullptr, 549);
    guildOk->Init(0, 0, 20, 10, nullptr, nullptr, nullptr, {}, nullptr, 550);
    auto* memberRaw = memberName.get();
    auto* danRankRaw = danRank.get();
    auto* guildRankRaw = guildRank.get();
    auto* danOkRaw = danOk.get();
    auto* guildOkRaw = guildOk.get();
    dialog.Add(std::move(memberName));
    dialog.Add(std::move(danRank));
    dialog.Add(std::move(guildRank));
    dialog.Add(std::move(danOk));
    dialog.Add(std::move(guildOk));
    dialog.Linking();
    EXPECT_EQ(dialog.memberNameControl(), memberRaw);
    EXPECT_EQ(dialog.danRankCombo(), danRankRaw);
    EXPECT_EQ(dialog.guildRankCombo(), guildRankRaw);
    EXPECT_EQ(dialog.danOkButton(), danOkRaw);
    EXPECT_EQ(dialog.guildOkButton(), guildOkRaw);
}

TEST(GuildRankDialogTest, LinkingRejectsWrongRuntimeTypes) {
    cGuildRankDialog dialog;
    auto wrong = std::make_unique<cStatic>();
    wrong->Init(0, 0, 20, 10, nullptr, cGuildRankDialog::kMemberNameId);
    dialog.Add(std::move(wrong));
    dialog.Linking();
    EXPECT_EQ(dialog.memberNameControl(), nullptr);
}

TEST(GuildRankDialogTest, TestInjectionPublishesControls) {
    cGuildRankDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    EXPECT_EQ(dialog.memberNameControl(), &controls.memberName);
    EXPECT_EQ(dialog.guildRankCombo(), &controls.guildRank);
    EXPECT_EQ(dialog.danRankCombo(), &controls.danRank);
    EXPECT_EQ(dialog.guildOkButton(), &controls.guildOk);
    EXPECT_EQ(dialog.danOkButton(), &controls.danOk);
}

TEST(GuildRankDialogTest, NonMaxLevelShowsDanControls) {
    cGuildRankDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.ShowGuildRankMode(4);
    EXPECT_EQ(dialog.currentMode(), static_cast<std::uint8_t>(GuildRankMode::Dan));
    EXPECT_TRUE(controls.danRank.isActive());
    EXPECT_TRUE(controls.danOk.isActive());
    EXPECT_FALSE(controls.guildRank.isActive());
    EXPECT_FALSE(controls.guildOk.isActive());
}

TEST(GuildRankDialogTest, MaxLevelShowsGuildControls) {
    cGuildRankDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.ShowGuildRankMode(cGuildRankDialog::kMaxGuildLevel);
    EXPECT_EQ(dialog.currentMode(), static_cast<std::uint8_t>(GuildRankMode::Guild));
    EXPECT_TRUE(controls.guildRank.isActive());
    EXPECT_TRUE(controls.guildOk.isActive());
}

TEST(GuildRankDialogTest, SwitchingModesDeactivatesPreviousControls) {
    cGuildRankDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.ShowGuildRankMode(1);
    dialog.ShowGuildRankMode(5);
    EXPECT_FALSE(controls.danRank.isActive());
    EXPECT_FALSE(controls.danOk.isActive());
    EXPECT_TRUE(controls.guildRank.isActive());
    EXPECT_TRUE(controls.guildOk.isActive());
}

TEST(GuildRankDialogTest, RepeatingCurrentModeIsLegacyNoOp) {
    cGuildRankDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.ShowGuildRankMode(5);
    controls.guildRank.SetActive(false);
    dialog.ShowGuildRankMode(5);
    EXPECT_FALSE(controls.guildRank.isActive());
}

TEST(GuildRankDialogTest, InvalidModeActivationIsIgnored) {
    cGuildRankDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    controls.danRank.SetActive(false);
    dialog.SetActiveGuildRankMode(-1, true);
    dialog.SetActiveGuildRankMode(2, true);
    EXPECT_FALSE(controls.danRank.isActive());
}

TEST(GuildRankDialogTest, ModeActivationToleratesMissingControls) {
    cGuildRankDialog dialog;
    dialog.ShowGuildRankMode(5);
    EXPECT_EQ(dialog.currentMode(), static_cast<std::uint8_t>(GuildRankMode::Guild));
}

TEST(GuildRankDialogTest, ValidSelectionFormatsNameAndActivates) {
    cGuildRankDialog dialog;
    Controls controls;
    ChatCapture capture;
    controls.Attach(dialog);
    dialog.SetSelection(ValidSelection());
    dialog.SetChatCallbacks(&ChatText, &SystemMessage, &capture);
    dialog.SetActive(true);
    EXPECT_TRUE(dialog.isActive());
    EXPECT_EQ(controls.memberName.GetScriptText(), "Member:Alice");
    EXPECT_EQ(capture.lastMessageId, cGuildRankDialog::kMemberNameFormatMessageId);
    EXPECT_EQ(capture.systemCalls, 0);
}

TEST(GuildRankDialogTest, ZeroSelectionRejectsAndEmitsSystemMessage) {
    cGuildRankDialog dialog;
    ChatCapture capture;
    dialog.SetSelection(GuildRankSelection{0, 99, ""});
    dialog.SetChatCallbacks(&ChatText, &SystemMessage, &capture);
    dialog.SetActive(true);
    EXPECT_FALSE(dialog.isActive());
    EXPECT_EQ(capture.lastMessageId, cGuildRankDialog::kInvalidSelectionMessageId);
    EXPECT_EQ(capture.systemCalls, 1);
    EXPECT_EQ(capture.systemMessage, "Invalid member");
}

TEST(GuildRankDialogTest, SelectingHeroRejectsActivation) {
    cGuildRankDialog dialog;
    ChatCapture capture;
    dialog.SetSelection(GuildRankSelection{99, 99, "Hero"});
    dialog.SetChatCallbacks(&ChatText, &SystemMessage, &capture);
    dialog.SetActive(true);
    EXPECT_FALSE(dialog.isActive());
    EXPECT_EQ(capture.systemCalls, 1);
}

TEST(GuildRankDialogTest, InvalidSelectionClosesAlreadyActiveDialog) {
    cGuildRankDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.SetSelection(ValidSelection());
    dialog.SetActive(true);
    ASSERT_TRUE(dialog.isActive());
    dialog.SetSelection(GuildRankSelection{0, 99, ""});
    dialog.SetActive(true);
    EXPECT_FALSE(dialog.isActive());
}

TEST(GuildRankDialogTest, DeactivationDoesNotRequireSelection) {
    cGuildRankDialog dialog;
    dialog.cDialog::SetActive(true);
    dialog.SetActive(false);
    EXPECT_FALSE(dialog.isActive());
}

TEST(GuildRankDialogTest, SetNameUsesFallbackFormatWithoutChatManager) {
    cGuildRankDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.SetName("Alice");
    EXPECT_EQ(controls.memberName.GetScriptText(), "Alice");
}

TEST(GuildRankDialogTest, SetNameAcceptsNullAndMissingControl) {
    cGuildRankDialog dialog;
    dialog.SetName(nullptr);
    Controls controls;
    controls.Attach(dialog);
    dialog.SetName(nullptr);
    EXPECT_EQ(controls.memberName.GetScriptText(), "");
}

TEST(GuildRankDialogTest, InvalidSelectionWithoutSystemCallbackIsSafe) {
    cGuildRankDialog dialog;
    dialog.SetSelection(GuildRankSelection{});
    dialog.SetActive(true);
    EXPECT_FALSE(dialog.isActive());
}
