#include <gtest/gtest.h>
#include <mxh/server/help_request_manager.hpp>

using namespace mxh::server;

TEST(HelpRequest, NoneNeverRequests) {
    HelpRequestMonster monster{1, 1, 100, HelpRequestType::none};
    EXPECT_FALSE(monster_request_process(monster).requested);
}

TEST(HelpRequest, OneTimeHp50UsesStrictLessAndClears) {
    HelpRequestMonster monster{1, 49, 100, HelpRequestType::one_time_if_hp50};
    auto result = monster_request_process(monster);
    EXPECT_TRUE(result.requested);
    EXPECT_TRUE(result.clearHelpType);
    EXPECT_EQ(monster.helpType, HelpRequestType::none);
}

TEST(HelpRequest, OneTimeHp50AtHalfDoesNotRequest) {
    HelpRequestMonster monster{1, 50, 100, HelpRequestType::one_time_if_hp50};
    EXPECT_FALSE(monster_request_process(monster).requested);
}

TEST(HelpRequest, AlwaysHp30UsesFractionThreshold) {
    HelpRequestMonster monster{1, 29, 100, HelpRequestType::always_if_hp30};
    EXPECT_TRUE(monster_request_process(monster).requested);
    monster.life = 30;
    EXPECT_FALSE(monster_request_process(monster).requested);
}

TEST(HelpRequest, DieOnlyRequestsAtZero) {
    HelpRequestMonster monster{1, 0, 100, HelpRequestType::die};
    EXPECT_TRUE(monster_request_process(monster).requested);
    monster.life = 1;
    EXPECT_FALSE(monster_request_process(monster).requested);
}

TEST(HelpRequest, AlwaysRequestsEveryCall) {
    HelpRequestMonster monster{1, 100, 100, HelpRequestType::always};
    EXPECT_TRUE(monster_request_process(monster).requested);
}

TEST(HelpRequest, HelperLegacyKindConditionRejectsBothKinds) {
    HelperMonsterState state{};
    state.askerPresent = true;
    state.helperPresent = true;
    state.objectKind = 1;
    state.helperGrid = state.targetGrid = 7;
    state.targetChange = true;
    EXPECT_FALSE(set_helper_monster(state));
    state.objectKind = 2;
    EXPECT_FALSE(set_helper_monster(state));
}

TEST(HelpRequest, HelperLegacyKindConditionMakesAssignmentUnreachable) {
    HelperMonsterState state{};
    state.askerPresent = true;
    state.helperPresent = true;
    state.helperGrid = state.targetGrid = 7;
    state.targetChange = true;
    state.monsterKind = 1;
    state.objectKind = 0;
    EXPECT_FALSE(set_helper_monster(state));
    state.objectKind = 1;
    EXPECT_FALSE(set_helper_monster(state));
    state.objectKind = 2;
    EXPECT_FALSE(set_helper_monster(state));
}
