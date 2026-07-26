#include "cquestdialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(QuestDialog, AddsSelectsAndClaimsCompletedQuest){cQuestDialog d;d.AddQuest({1,"First",QuestStatus::Completed,100});ASSERT_TRUE(d.Select(0));QuestEntry got{};d.SetClaimCallback([&](const auto&q){got=q;return true;});EXPECT_TRUE(d.ClaimSelected());EXPECT_EQ(got.reward,100u);EXPECT_EQ(d.Quests()[0].status,QuestStatus::Claimed);}
TEST(QuestDialog, RejectsUnavailableClaimAndDuplicateQuest){cQuestDialog d;d.AddQuest({1,"First",QuestStatus::Active,1});d.AddQuest({1,"Dup",QuestStatus::Completed,2});EXPECT_EQ(d.Quests().size(),1u);d.Select(0);EXPECT_FALSE(d.ClaimSelected());}
TEST(QuestDialog, UpdatesAndRejectsUnknownQuest){cQuestDialog d;d.AddQuest({1,"First",QuestStatus::Available,1});EXPECT_TRUE(d.UpdateQuest(1,QuestStatus::Active));EXPECT_FALSE(d.UpdateQuest(9,QuestStatus::Completed));}
