// quest_manager_test.cpp

#include "mxh/server/quest_manager.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::QuestState;
using mxh::server::QuestSubKind;
using mxh::server::QuestSub;
using mxh::server::QuestDefinition;
using mxh::server::QuestProgress;
using mxh::server::QuestLog;
using mxh::server::start_quest;
using mxh::server::increment_sub;
using mxh::server::evaluate_quest_state;
using mxh::server::complete_quest;
using mxh::server::fail_quest;
using mxh::server::reward_quest;
using mxh::server::accept_quest;

static QuestDefinition make_kill_quest(std::uint32_t id, std::uint32_t monster_kind, std::uint32_t count) {
    QuestDefinition d;
    d.quest_id = id;
    d.title = "Kill Quest";
    d.reward_exp = 1000;
    d.reward_money = 500;
    QuestSub s;
    s.kind = QuestSubKind::Kill;
    s.target_id = monster_kind;
    s.target = count;
    d.subs.push_back(s);
    return d;
}
}

// ---- start_quest ----
TEST(StartQuest, InitializedToAccepted) {
    auto def = make_kill_quest(101, 200, 5);
    auto p = start_quest(1001, def, 12345);
    EXPECT_EQ(p.player_id, 1001u);
    EXPECT_EQ(p.quest_id, 101u);
    EXPECT_EQ(p.state, QuestState::Accepted);
    EXPECT_EQ(p.accepted_time_ms, 12345u);
    EXPECT_EQ(p.subs.size(), 1u);
}

TEST(StartQuest, SubsCopiedFromDefinition) {
    auto def = make_kill_quest(101, 200, 5);
    auto p = start_quest(1001, def, 0);
    EXPECT_EQ(p.subs[0].kind, QuestSubKind::Kill);
    EXPECT_EQ(p.subs[0].target_id, 200u);
    EXPECT_EQ(p.subs[0].target, 5u);
    EXPECT_EQ(p.subs[0].count, 0u);
}
// ---- increment_sub ----
TEST(IncrementSub, KillCountGrows) {
    auto def = make_kill_quest(101, 200, 5);
    auto p = start_quest(1, def, 0);
    EXPECT_FALSE(increment_sub(p, QuestSubKind::Kill, 200, 1));
    EXPECT_EQ(p.subs[0].count, 1u);
}

TEST(IncrementSub, ReachingTargetReturnsTrue) {
    auto def = make_kill_quest(101, 200, 5);
    auto p = start_quest(1, def, 0);
    EXPECT_TRUE(increment_sub(p, QuestSubKind::Kill, 200, 5));
    EXPECT_EQ(p.subs[0].count, 5u);
}

TEST(IncrementSub, BeyondTargetClampsToTarget) {
    auto def = make_kill_quest(101, 200, 5);
    auto p = start_quest(1, def, 0);
    increment_sub(p, QuestSubKind::Kill, 200, 10);
    EXPECT_EQ(p.subs[0].count, 5u);  // clamped
}

TEST(IncrementSub, UnknownTargetIsNoOp) {
    auto def = make_kill_quest(101, 200, 5);
    auto p = start_quest(1, def, 0);
    EXPECT_FALSE(increment_sub(p, QuestSubKind::Kill, 999, 1));
    EXPECT_EQ(p.subs[0].count, 0u);
}

TEST(IncrementSub, DifferentKindIsNoOp) {
    auto def = make_kill_quest(101, 200, 5);
    auto p = start_quest(1, def, 0);
    EXPECT_FALSE(increment_sub(p, QuestSubKind::Collect, 200, 1));
}

// ---- evaluate_quest_state ----
TEST(EvaluateQuest, StaysAcceptedWhileIncomplete) {
    auto def = make_kill_quest(101, 200, 5);
    auto p = start_quest(1, def, 0);
    increment_sub(p, QuestSubKind::Kill, 200, 2);
    EXPECT_EQ(evaluate_quest_state(p), QuestState::Accepted);
}

TEST(EvaluateQuest, BecomesCompleteWhenAllDone) {
    auto def = make_kill_quest(101, 200, 5);
    auto p = start_quest(1, def, 0);
    increment_sub(p, QuestSubKind::Kill, 200, 5);
    EXPECT_EQ(evaluate_quest_state(p), QuestState::Complete);
}

TEST(EvaluateQuest, CompleteIsIdempotent) {
    auto def = make_kill_quest(101, 200, 5);
    auto p = start_quest(1, def, 0);
    increment_sub(p, QuestSubKind::Kill, 200, 5);
    EXPECT_EQ(evaluate_quest_state(p), QuestState::Complete);
    EXPECT_EQ(evaluate_quest_state(p), QuestState::Complete);
}

// ---- state transitions ----
TEST(CompleteQuest, SetsCompleteFromAccepted) {
    auto def = make_kill_quest(101, 200, 5);
    auto p = start_quest(1, def, 0);
    EXPECT_EQ(complete_quest(p), QuestState::Complete);
    EXPECT_EQ(p.state, QuestState::Complete);
}

TEST(RewardQuest, OnlyFromComplete) {
    auto def = make_kill_quest(101, 200, 5);
    auto p = start_quest(1, def, 0);
    EXPECT_EQ(reward_quest(p), QuestState::Accepted);  // not complete
    complete_quest(p);
    EXPECT_EQ(reward_quest(p), QuestState::Rewarded);
}

TEST(FailQuest, SetsFailed) {
    auto def = make_kill_quest(101, 200, 5);
    auto p = start_quest(1, def, 0);
    EXPECT_EQ(fail_quest(p), QuestState::Failed);
}

// ---- QuestLog ----
TEST(QuestLog, AcceptQuestAddsToLog) {
    QuestLog log; log.player_id = 1001;
    auto def = make_kill_quest(101, 200, 5);
    EXPECT_TRUE(accept_quest(log, def, 12345));
    EXPECT_EQ(log.quests.size(), 1u);
}

TEST(QuestLog, AcceptDuplicateReturnsFalse) {
    QuestLog log; log.player_id = 1001;
    auto def = make_kill_quest(101, 200, 5);
    accept_quest(log, def, 0);
    EXPECT_FALSE(accept_quest(log, def, 0));
    EXPECT_EQ(log.quests.size(), 1u);
}

TEST(QuestLog, AcceptMultipleDifferentQuests) {
    QuestLog log; log.player_id = 1001;
    auto def1 = make_kill_quest(101, 200, 5);
    auto def2 = make_kill_quest(102, 201, 3);
    EXPECT_TRUE(accept_quest(log, def1, 0));
    EXPECT_TRUE(accept_quest(log, def2, 0));
    EXPECT_EQ(log.quests.size(), 2u);
}

TEST(QuestLog, FindQuestReturnsPointerWhenPresent) {
    QuestLog log; log.player_id = 1001;
    accept_quest(log, make_kill_quest(101, 200, 5), 0);
    accept_quest(log, make_kill_quest(102, 201, 3), 0);
    auto found = find_quest(log, 102);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ((*found)->quest_id, 102u);
}

TEST(QuestLog, FindQuestAbsentReturnsNullopt) {
    QuestLog log; log.player_id = 1001;
    accept_quest(log, make_kill_quest(101, 200, 5), 0);
    EXPECT_FALSE(find_quest(log, 999).has_value());
}

// ---- End-to-end ----
TEST(QuestEndToEnd, KillThreeOfFiveCompletes) {
    QuestLog log; log.player_id = 1001;
    auto def = make_kill_quest(101, 200, 5);
    accept_quest(log, def, 0);

    auto q = find_quest(log, 101);
    ASSERT_TRUE(q.has_value());

    EXPECT_EQ(evaluate_quest_state(*q.value()), QuestState::Accepted);
    increment_sub(*q.value(), QuestSubKind::Kill, 200, 3);
    EXPECT_EQ(evaluate_quest_state(*q.value()), QuestState::Accepted);

    increment_sub(*q.value(), QuestSubKind::Kill, 200, 2);
    EXPECT_EQ(evaluate_quest_state(*q.value()), QuestState::Complete);

    EXPECT_EQ(reward_quest(*q.value()), QuestState::Rewarded);
}

TEST(QuestDefinition, CanHaveMultipleSubs) {
    QuestDefinition d; d.quest_id = 200;
    QuestSub s1; s1.kind = QuestSubKind::Kill; s1.target_id = 100; s1.target = 3;
    QuestSub s2; s2.kind = QuestSubKind::Collect; s2.target_id = 42; s2.target = 10;
    d.subs.push_back(s1); d.subs.push_back(s2);
    auto p = start_quest(1, d, 0);
    EXPECT_EQ(p.subs.size(), 2u);
    // Only Kill sub complete -> still Accepted
    increment_sub(p, QuestSubKind::Kill, 100, 3);
    EXPECT_EQ(evaluate_quest_state(p), QuestState::Accepted);
    // Both complete -> Complete
    increment_sub(p, QuestSubKind::Collect, 42, 10);
    EXPECT_EQ(evaluate_quest_state(p), QuestState::Complete);
}

// ---- drop_quest ----
TEST(DropQuest, RemovesActiveQuest) {
    auto def = make_kill_quest(101, 200, 5);
    QuestLog log; log.player_id = 1;
    accept_quest(log, def, 0);
    EXPECT_EQ(log.quests.size(), 1u);
    EXPECT_TRUE(drop_quest(log, 101));
    EXPECT_EQ(log.quests.size(), 0u);
}

TEST(DropQuest, UnknownQuestReturnsFalse) {
    QuestLog log;
    EXPECT_FALSE(drop_quest(log, 999));
}

TEST(DropQuest, RewardedQuestRefusesToDrop) {
    auto def = make_kill_quest(101, 200, 5);
    QuestLog log; log.player_id = 1;
    accept_quest(log, def, 0);
    auto p = find_quest(log, 101);
    ASSERT_TRUE(p.has_value());
    increment_sub(*p.value(), QuestSubKind::Kill, 200, 5);
    evaluate_quest_state(*p.value());
    reward_quest(*p.value());
    EXPECT_FALSE(drop_quest(log, 101));
    EXPECT_EQ(log.quests.size(), 1u);
}

// ---- summarize ----
TEST(Summarize, CountsByState) {
    QuestLog log; log.player_id = 1;
    auto d1 = make_kill_quest(101, 200, 5);
    auto d2 = make_kill_quest(102, 200, 3);
    auto d3 = make_kill_quest(103, 200, 2);
    accept_quest(log, d1, 0);
    accept_quest(log, d2, 0);
    accept_quest(log, d3, 0);
    auto p3 = find_quest(log, 103);
    increment_sub(*p3.value(), QuestSubKind::Kill, 200, 2);
    evaluate_quest_state(*p3.value());
    reward_quest(*p3.value());
    auto p2 = find_quest(log, 102);
    fail_quest(*p2.value());
    auto s = summarize(log);
    EXPECT_EQ(s.accepted, 1u);
    EXPECT_EQ(s.rewarded, 1u);
    EXPECT_EQ(s.failed, 1u);
    EXPECT_EQ(s.complete, 0u);
}

// ---- accept_quest duplicate prevention ----
TEST(AcceptQuest, DuplicateIsRejected) {
    auto def = make_kill_quest(101, 200, 5);
    QuestLog log; log.player_id = 1;
    EXPECT_TRUE(accept_quest(log, def, 0));
    EXPECT_FALSE(accept_quest(log, def, 100));
    EXPECT_EQ(log.quests.size(), 1u);
}

// ---- evaluate_quest_state promotes Accepted->Complete when all subs hit ----
TEST(EvaluateQuest, PromotesAcceptedToComplete) {
    auto def = make_kill_quest(101, 200, 5);
    auto p = start_quest(1, def, 0);
    EXPECT_EQ(p.state, QuestState::Accepted);
    increment_sub(p, QuestSubKind::Kill, 200, 5);
    EXPECT_EQ(evaluate_quest_state(p), QuestState::Complete);
    EXPECT_EQ(p.state, QuestState::Complete);
}

// ---- reward_quest only valid from Complete ----
TEST(RewardQuest, OnlyCompletesToRewarded) {
    auto def = make_kill_quest(101, 200, 5);
    auto p = start_quest(1, def, 0);
    EXPECT_EQ(reward_quest(p), QuestState::Accepted);
    increment_sub(p, QuestSubKind::Kill, 200, 5);
    evaluate_quest_state(p);
    EXPECT_EQ(reward_quest(p), QuestState::Rewarded);
}
