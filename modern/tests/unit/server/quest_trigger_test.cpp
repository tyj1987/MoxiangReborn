#include "mxh/server/quest_trigger.hpp"
#include "mxh/server/quest_execute.hpp"
#include "mxh/server/quest_group.hpp"
#include "mxh/server/quest_script_line.hpp"

#include <gtest/gtest.h>

namespace {
using mxh::server::QuestEventKind;
using mxh::server::QuestEventSpec;
using mxh::server::QuestExecuteKind;
using mxh::server::QuestScriptLine;
using mxh::server::QuestTrigger;
using mxh::server::QuestTriggerApplyStatus;
using mxh::server::parse_quest_execute;
using mxh::server::parse_quest_script_line;
using mxh::server::quest_trigger_check;
using mxh::server::quest_trigger_from_script_line;
using mxh::server::quest_trigger_run;
using mxh::server::make_quest_group;
using mxh::server::quest_group_create_quest;
using mxh::server::quest_group_start_subquest;
using mxh::server::quest_group_set_item;
}

QuestScriptLine make_line(std::uint32_t quest_idx, std::uint32_t sub_idx,
                          const char* event_token, std::uint32_t p1, int p2,
                          const std::vector<const char*>& execute_strs) {
    std::string src;
    src += "&LEVEL 1 99 ";
    src += event_token;
    src += " ";
    src += std::to_string(p1);
    src += " ";
    src += std::to_string(p2);
    for (const auto* ex : execute_strs) {
        src += " ";
        src += ex;
    }
    const auto line = parse_quest_script_line(src, quest_idx, sub_idx);
    return line.value();
}

TEST(QuestTriggerBuild, CopiesEventAndExecutesFromScriptLine) {
    auto line = make_line(7u, 2u, "@HUNT", 1u, 10,
                          {"*ADDCOUNT 1 5", "*ENDSUB"});
    const auto trigger = quest_trigger_from_script_line(line);
    EXPECT_EQ(trigger.quest_idx, 7u);
    EXPECT_EQ(trigger.subquest_idx, 2u);
    EXPECT_EQ(trigger.event.kind, QuestEventKind::Hunt);
    EXPECT_EQ(trigger.event.param1, 1u);
    EXPECT_EQ(trigger.event.param2, 10);
    ASSERT_EQ(trigger.executes.size(), 2u);
    EXPECT_EQ(trigger.executes[0].kind, QuestExecuteKind::AddCount);
    EXPECT_EQ(trigger.executes[1].kind, QuestExecuteKind::EndSub);
    EXPECT_EQ(trigger.end_param, 0u);
}

TEST(QuestTriggerBuild, EndParamCapturesFirstEndQuestSubquest) {
    const std::string src = "&LEVEL 1 99 @HUNT 1 10 *ENDQUEST 7 *ENDSUB";
    const auto line = parse_quest_script_line(src, 5u, 0u);
    ASSERT_TRUE(line.has_value());
    const auto trigger = quest_trigger_from_script_line(*line);
    EXPECT_EQ(trigger.end_param, 7u);
    EXPECT_EQ(trigger.executes[0].kind, QuestExecuteKind::EndQuest);
    ASSERT_EQ(trigger.executes[0].args.size(), 1u);
    EXPECT_EQ(trigger.executes[0].args[0], 7u);
}

TEST(QuestTriggerCheck, HuntAllMatchesAnyHunt) {
    const QuestEventSpec huntall{QuestEventKind::HuntAll, 0u, 0};
    QuestTrigger trigger;
    trigger.event = huntall;
    EXPECT_TRUE(quest_trigger_check(trigger, {QuestEventKind::Hunt, 5u, 5}));
    EXPECT_FALSE(quest_trigger_check(trigger, {QuestEventKind::Count, 5u, 5}));
}

TEST(QuestTriggerCheck, ExactMatchRequiredForNonHuntAll) {
    QuestTrigger trigger;
    trigger.event = {QuestEventKind::Hunt, 1u, 10};
    EXPECT_TRUE(quest_trigger_check(trigger, {QuestEventKind::Hunt, 1u, 10}));
    EXPECT_FALSE(quest_trigger_check(trigger, {QuestEventKind::Hunt, 2u, 10}));
    EXPECT_FALSE(quest_trigger_check(trigger, {QuestEventKind::Count, 1u, 10}));
}

TEST(QuestTriggerRun, AppliesAddCountWhenConditionMatches) {
    auto state = make_quest_group();
    ASSERT_TRUE(quest_group_create_quest(state, 7u));
    const std::string src = "&LEVEL 1 99 @HUNT 1 10 *ADDCOUNT 2 3";
    const auto line = parse_quest_script_line(src, 7u, 0u);
    ASSERT_TRUE(line.has_value());
    const auto trigger = quest_trigger_from_script_line(*line);
    const QuestEventSpec runtime{QuestEventKind::Hunt, 1u, 10};
    const auto result = quest_trigger_run(trigger, runtime, state);
    EXPECT_EQ(result.status, QuestTriggerApplyStatus::Applied);
    EXPECT_TRUE(result.changed);
    EXPECT_EQ(state.m_QuestTable.at(7u).subQuestData.at(2u), 1u);
}

TEST(QuestTriggerRun, ConditionFailedDoesNotApply) {
    auto state = make_quest_group();
    ASSERT_TRUE(quest_group_create_quest(state, 7u));
    const std::string src = "&LEVEL 1 99 @HUNT 1 10 *ADDCOUNT 2 3";
    const auto line = parse_quest_script_line(src, 7u, 0u);
    ASSERT_TRUE(line.has_value());
    const auto trigger = quest_trigger_from_script_line(*line);
    const QuestEventSpec runtime{QuestEventKind::Hunt, 2u, 10};
    const auto result = quest_trigger_run(trigger, runtime, state);
    EXPECT_EQ(result.status, QuestTriggerApplyStatus::ConditionFailed);
    EXPECT_FALSE(result.changed);
    EXPECT_EQ(state.m_QuestTable.at(7u).subQuestData.count(2u), 0u);
}

TEST(QuestTriggerRun, AppliesEndSubAfterStart) {
    auto state = make_quest_group();
    ASSERT_TRUE(quest_group_create_quest(state, 7u));
    ASSERT_TRUE(quest_group_start_subquest(state, 7u, 2u, 0u));
    const std::string src = "&LEVEL 1 99 @HUNT 1 10 *ENDSUB";
    const auto line = parse_quest_script_line(src, 7u, 2u);
    ASSERT_TRUE(line.has_value());
    const auto trigger = quest_trigger_from_script_line(*line);
    const QuestEventSpec runtime{QuestEventKind::Hunt, 1u, 10};
    const auto result = quest_trigger_run(trigger, runtime, state, 0, 0, 1000u);
    EXPECT_EQ(result.status, QuestTriggerApplyStatus::Applied);
    EXPECT_EQ(state.m_QuestTable.at(7u).activeSubquests.count(2u), 0u);
}

TEST(QuestTriggerRun, GiveQuestItemRemoveHonorsApplyResult) {
    auto state = make_quest_group();
    ASSERT_TRUE(quest_group_create_quest(state, 7u));
    quest_group_set_item(state, 7u, 100u, 2u);
    const std::string src = "&LEVEL 1 99 @USEITEM 1 10 *GIVEQUESTITEM 100 0";
    const auto line = parse_quest_script_line(src, 7u, 0u);
    ASSERT_TRUE(line.has_value());
    const auto trigger = quest_trigger_from_script_line(*line);
    const QuestEventSpec runtime{QuestEventKind::UseItem, 1u, 10};
    const auto result = quest_trigger_run(trigger, runtime, state);
    EXPECT_EQ(result.status, QuestTriggerApplyStatus::Applied);
    EXPECT_EQ(state.m_QuestItemTable.count(100u), 0u);
}

TEST(QuestTriggerRun, MissingExecuteReportsFailed) {
    auto state = make_quest_group();
    const std::string src = "&LEVEL 1 99 @HUNT 1 10 *ENDSUB";
    const auto line = parse_quest_script_line(src, 7u, 5u);
    ASSERT_TRUE(line.has_value());
    const auto trigger = quest_trigger_from_script_line(*line);
    const QuestEventSpec runtime{QuestEventKind::Hunt, 1u, 10};
    const auto result = quest_trigger_run(trigger, runtime, state);
    EXPECT_EQ(result.status, QuestTriggerApplyStatus::ExecuteFailed);
}

TEST(QuestTriggerRun, MultipleExecutesAllApply) {
    auto state = make_quest_group();
    ASSERT_TRUE(quest_group_create_quest(state, 7u));
    ASSERT_TRUE(quest_group_start_subquest(state, 7u, 2u, 0u));
    const std::string src = "&LEVEL 1 99 @HUNT 1 10 *ADDCOUNT 2 3 *ENDSUB";
    const auto line = parse_quest_script_line(src, 7u, 2u);
    ASSERT_TRUE(line.has_value());
    const auto trigger = quest_trigger_from_script_line(*line);
    const QuestEventSpec runtime{QuestEventKind::Hunt, 1u, 10};
    const auto result = quest_trigger_run(trigger, runtime, state, 0, 0, 1000u);
    EXPECT_EQ(result.status, QuestTriggerApplyStatus::Applied);
    EXPECT_EQ(state.m_QuestTable.at(7u).activeSubquests.count(2u), 0u);
    ASSERT_EQ(trigger.executes.size(), 2u);
    EXPECT_EQ(trigger.executes[1].kind, QuestExecuteKind::EndSub);
}
