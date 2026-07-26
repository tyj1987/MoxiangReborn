#include <mxh/server/quest_updater.hpp>
#include <gtest/gtest.h>

using namespace mxh::server;

namespace {
const QuestUpdaterQuestView quest{77, 0x55, 1234};
}

TEST(QuestUpdaterNull, QuestPointerOperationsIgnoreNull) {
    EXPECT_FALSE(quest_updater_start_quest(1, nullptr).has_value());
    EXPECT_FALSE(quest_updater_end_quest(1, nullptr).has_value());
    EXPECT_FALSE(quest_updater_delete_quest(1, nullptr).has_value());
    EXPECT_FALSE(quest_updater_start_subquest(1, nullptr, 2, {}).has_value());
    EXPECT_FALSE(quest_updater_end_subquest(1, nullptr, 2).has_value());
    EXPECT_FALSE(quest_updater_update_subquest(1, nullptr, 2, {}).has_value());
}

TEST(QuestUpdaterStart, MapsMainQuestInsertFields) {
    const auto command = quest_updater_start_quest(9, &quest).value();
    EXPECT_EQ(command.kind, QuestDbCommandKind::MainQuestInsert);
    EXPECT_EQ(command.playerId, 9u);
    EXPECT_EQ(command.questIdx, 77u);
    EXPECT_EQ(command.flag, 0x55u);
    EXPECT_EQ(command.time, 1234u);
}

TEST(QuestUpdaterEnd, ForcesCompletionToOne) {
    const auto command = quest_updater_end_quest(9, &quest).value();
    EXPECT_EQ(command.kind, QuestDbCommandKind::EndQuestNew);
    EXPECT_EQ(command.completion, 1u);
    EXPECT_EQ(command.flag, 0x55u);
    EXPECT_EQ(command.time, 1234u);
}

TEST(QuestUpdaterDelete, UsesNewDeleteCommandOnly) {
    const auto command = quest_updater_delete_quest(9, &quest).value();
    EXPECT_EQ(command.kind, QuestDbCommandKind::DeleteQuestNew);
    EXPECT_EQ(command.playerId, 9u);
    EXPECT_EQ(command.questIdx, 77u);
    EXPECT_EQ(command.flag, 0u);
}

TEST(QuestUpdaterUpdate, LegacyMainUpdateUsesQuestIndexZero) {
    const auto command = quest_updater_update_quest(9, 5, 6, 7);
    EXPECT_EQ(command.kind, QuestDbCommandKind::MainQuestUpdate);
    EXPECT_EQ(command.questIdx, 0u);
    EXPECT_EQ(command.flag, 5u);
    EXPECT_EQ(command.param, 6u);
    EXPECT_EQ(command.time, 7u);
}

TEST(QuestUpdaterSubStart, MapsDataAndTime) {
    const auto command = quest_updater_start_subquest(9, &quest, 3, {44, 55}).value();
    EXPECT_EQ(command.kind, QuestDbCommandKind::SubQuestInsert);
    EXPECT_EQ(command.questIdx, 77u);
    EXPECT_EQ(command.subQuestIdx, 3u);
    EXPECT_EQ(command.param, 44u);
    EXPECT_EQ(command.time, 55u);
}

TEST(QuestUpdaterSubEnd, UsesParentFlagAndTime) {
    const auto command = quest_updater_end_subquest(9, &quest, 3).value();
    EXPECT_EQ(command.kind, QuestDbCommandKind::EndSubQuestNew);
    EXPECT_EQ(command.subQuestIdx, 3u);
    EXPECT_EQ(command.flag, 0x55u);
    EXPECT_EQ(command.time, 1234u);
}

TEST(QuestUpdaterSubUpdate, MapsCurrentDataAndTime) {
    const auto command = quest_updater_update_subquest(9, &quest, 3, {88, 99}).value();
    EXPECT_EQ(command.kind, QuestDbCommandKind::SubQuestUpdate);
    EXPECT_EQ(command.param, 88u);
    EXPECT_EQ(command.time, 99u);
}

TEST(QuestUpdaterGiveItem, PreservesLegacyDeleteSemantics) {
    const auto command = quest_updater_give_quest_item(9, 100, 999);
    EXPECT_EQ(command.kind, QuestDbCommandKind::QuestItemDelete);
    EXPECT_EQ(command.playerId, 9u);
    EXPECT_EQ(command.itemIdx, 100u);
    EXPECT_EQ(command.itemNum, 0u);
}

TEST(QuestUpdaterTakeItem, PreservesLegacyInsertSemantics) {
    const auto command = quest_updater_take_quest_item(9, 77, 100, 3);
    EXPECT_EQ(command.kind, QuestDbCommandKind::QuestItemInsert);
    EXPECT_EQ(command.questIdx, 77u);
    EXPECT_EQ(command.itemIdx, 100u);
    EXPECT_EQ(command.itemNum, 3u);
}

TEST(QuestUpdaterItemUpdate, MapsAllFields) {
    const auto command = quest_updater_update_quest_item(9, 77, 100, 4);
    EXPECT_EQ(command.kind, QuestDbCommandKind::QuestItemUpdate);
    EXPECT_EQ(command.playerId, 9u);
    EXPECT_EQ(command.questIdx, 77u);
    EXPECT_EQ(command.itemIdx, 100u);
    EXPECT_EQ(command.itemNum, 4u);
}

TEST(QuestUpdaterCheckTime, MapsCheckTypeAndTime) {
    const auto command = quest_updater_update_check_time(9, 77, 2, 888);
    EXPECT_EQ(command.kind, QuestDbCommandKind::MainQuestUpdateCheckTime);
    EXPECT_EQ(command.playerId, 9u);
    EXPECT_EQ(command.questIdx, 77u);
    EXPECT_EQ(command.checkType, 2u);
    EXPECT_EQ(command.checkTime, 888u);
}

TEST(QuestUpdaterFields, UnusedFieldsRemainZero) {
    const auto command = quest_updater_delete_quest(9, &quest).value();
    EXPECT_EQ(command.itemIdx, 0u);
    EXPECT_EQ(command.itemNum, 0u);
    EXPECT_EQ(command.checkType, 0u);
    EXPECT_EQ(command.completion, 0u);
}
