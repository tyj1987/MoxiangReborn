#include "cquestdialog.hpp"
#include "mxh/ui/clistdialog.hpp"

#include <gtest/gtest.h>

#include <memory>

using namespace mxh::ui;

namespace {

struct QuestService final : mxh::services::IQuestService {
    std::uint32_t claimed{};
    bool allow{true};

    bool claimQuest(std::uint32_t id) override {
        if (!allow) return false;
        claimed = id;
        return true;
    }
};

}  // namespace

TEST(QuestDialog, AddsSelectsAndClaimsCompletedQuest) {
    cQuestDialog d;
    d.AddQuest({1, "First", QuestStatus::Completed, 100});
    ASSERT_TRUE(d.Select(0));
    QuestEntry got{};
    d.SetClaimCallback([&](const auto& q) {
        got = q;
        return true;
    });
    EXPECT_TRUE(d.ClaimSelected());
    EXPECT_EQ(got.reward, 100u);
    EXPECT_EQ(d.Quests()[0].status, QuestStatus::Claimed);
}

TEST(QuestDialog, RejectsUnavailableClaimAndDuplicateQuest) {
    cQuestDialog d;
    d.AddQuest({1, "First", QuestStatus::Active, 1});
    d.AddQuest({1, "Dup", QuestStatus::Completed, 2});
    EXPECT_EQ(d.Quests().size(), 1u);
    d.Select(0);
    EXPECT_FALSE(d.ClaimSelected());
}

TEST(QuestDialog, UpdatesAndRejectsUnknownQuest) {
    cQuestDialog d;
    d.AddQuest({1, "First", QuestStatus::Available, 1});
    EXPECT_TRUE(d.UpdateQuest(1, QuestStatus::Active));
    EXPECT_FALSE(d.UpdateQuest(9, QuestStatus::Completed));
}

TEST(QuestDialog, ClearQuestsRebuildsSelectionAndRuntimeList) {
    cQuestDialog d;
    d.AddQuest({1, "Old quest", QuestStatus::Active, 1});
    ASSERT_TRUE(d.Select(0));
    d.ClearQuests();
    EXPECT_TRUE(d.Quests().empty());
    EXPECT_EQ(d.Selected(), nullptr);
    d.AddQuest({2, "Fresh quest", QuestStatus::Available, 2});
    ASSERT_TRUE(d.Select(0));
    EXPECT_EQ(d.Selected()->id, 2u);
}

TEST(QuestDialog, RemoveQuestRebuildsListAndSelection) {
    cQuestDialog d;
    d.AddQuest({1, "First", QuestStatus::Active, 1});
    d.AddQuest({2, "Second", QuestStatus::Available, 2});
    ASSERT_TRUE(d.Select(1));
    EXPECT_TRUE(d.RemoveQuest(1));
    ASSERT_EQ(d.Quests().size(), 1u);
    ASSERT_NE(d.Selected(), nullptr);
    EXPECT_EQ(d.Selected()->id, 2u);
    EXPECT_FALSE(d.RemoveQuest(9));
}

TEST(QuestDialog, ServiceBackedClaimRequiresServiceAcceptance) {
    cQuestDialog d;
    QuestService service;
    d.SetQuestService(&service);
    d.AddQuest({7, "Service quest", QuestStatus::Completed, 99});
    ASSERT_TRUE(d.Select(0));
    EXPECT_TRUE(d.ClaimSelected());
    EXPECT_EQ(service.claimed, 7u);
    EXPECT_EQ(d.Selected()->status, QuestStatus::Claimed);
}

TEST(QuestDialog, ServiceBackedClaimPreservesCompletedStateOnRejection) {
    cQuestDialog d;
    QuestService service;
    service.allow = false;
    d.SetQuestService(&service);
    d.AddQuest({7, "Service quest", QuestStatus::Completed, 99});
    ASSERT_TRUE(d.Select(0));
    EXPECT_FALSE(d.ClaimSelected());
    EXPECT_EQ(d.Selected()->status, QuestStatus::Completed);
}

TEST(QuestDialog, LinkingPopulatesRuntimeQuestListAndSelection) {
    cQuestDialog d;
    d.AddQuest({10, "First live quest", QuestStatus::Available, 100});
    d.AddQuest({20, "Second live quest", QuestStatus::Active, 200});

    auto list = std::make_unique<cListDialog>();
    auto* listPtr = list.get();
    listPtr->setLegacyId("QUE_QUESTLIST");
    d.Add(std::move(list));

    d.Linking();
    ASSERT_EQ(d.QuestList(), listPtr);
    ASSERT_EQ(listPtr->RowCount(), 2u);
    EXPECT_EQ(listPtr->GetRow(0).first, "First live quest");
    EXPECT_EQ(listPtr->GetRow(1).first, "Second live quest");

    ASSERT_TRUE(d.Select(1));
    EXPECT_EQ(d.Selected()->id, 20u);
    EXPECT_EQ(listPtr->GetCurSelectedRowIdx(), 1);

    ASSERT_TRUE(d.UpdateQuest(20, QuestStatus::Completed));
    EXPECT_EQ(listPtr->RowCount(), 2u);
    EXPECT_EQ(listPtr->GetCurSelectedRowIdx(), 1);
}

TEST(QuestDialog, ListClickUpdatesSelectedQuest) {
    cQuestDialog d;
    d.Init(0, 0, 252, 365, nullptr);
    d.SetActive(true);
    d.AddQuest({10, "First live quest", QuestStatus::Available, 100});
    d.AddQuest({20, "Second live quest", QuestStatus::Active, 200});

    auto list = std::make_unique<cListDialog>();
    auto* listPtr = list.get();
    listPtr->Init(0, 0, 200, 145, nullptr);
    listPtr->InitList(10, 0, 0, 200, 145);
    listPtr->SetActive(true);
    listPtr->setLegacyId("QUE_QUESTLIST");
    d.Add(std::move(list));
    d.Linking();

    const auto event = d.ActionEvent(
        10, 15, cWindow::MouseFlagLButton);
    EXPECT_EQ(event, static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick));
    ASSERT_NE(d.Selected(), nullptr);
    EXPECT_EQ(d.Selected()->id, 20u);
    EXPECT_EQ(listPtr->GetCurSelectedRowIdx(), 1);
}
