// D4.114 -- AgentWeather side-effect plan unit tests.
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_WEATHERUserMsgParser (lines 5016-5034).
//

#include <gtest/gtest.h>

#include "mxh/server/agent_weather.hpp"
#include "mxh/server/agent_weather_side_effect_plan.hpp"

using namespace mxh::server;

TEST(WeatherPlan, DropNoUserEmitsDropEffect) {
    WeatherAction a{};
    a.kind = WeatherActionKind::drop_no_user;
    a.protocol = weather_set;
    a.connection_index = 11u;
    const auto plan = weather_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, WeatherSideEffectKind::Drop);
    EXPECT_FALSE(weather_effect_targets_map(plan.effects[0]));
}

TEST(WeatherPlan, DropWrongUserLevelEmitsDropEffect) {
    WeatherAction a{};
    a.kind = WeatherActionKind::drop_wrong_user_level;
    a.protocol = weather_exe;
    a.connection_index = 11u;
    const auto plan = weather_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, WeatherSideEffectKind::Drop);
}

TEST(WeatherPlan, ForwardToMapByDataEmitsMapForward) {
    WeatherAction a{};
    a.kind = WeatherActionKind::forward_to_map_by_data;
    a.protocol = weather_exe;
    a.connection_index = 11u;
    a.target_map = 17u;
    const auto plan = weather_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, WeatherSideEffectKind::ForwardToMapByData);
    EXPECT_EQ(plan.effects[0].target_map, 17u);
    EXPECT_TRUE(weather_effect_targets_map(plan.effects[0]));
}

TEST(WeatherClassifierPlan, NoUserEmitsDropPlan) {
    WeatherRequest req{};
    req.protocol = weather_set;
    req.user_found = false;
    const auto action = classify_weather(req);
    EXPECT_EQ(action.kind, WeatherActionKind::drop_no_user);
    const auto plan = weather_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(WeatherClassifierPlan, NonGmUserEmitsDropPlan) {
    WeatherRequest req{};
    req.protocol = weather_set;
    req.user_found = true;
    req.user_level = 0u;
    const auto action = classify_weather(req);
    EXPECT_EQ(action.kind, WeatherActionKind::drop_wrong_user_level);
    const auto plan = weather_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(WeatherClassifierPlan, GmClassWithoutMasterOrEventerDrops) {
    WeatherRequest req{};
    req.protocol = weather_set;
    req.user_found = true;
    req.user_level = user_level_gm_weather;
    req.is_gm_master_or_eventer = false;
    const auto action = classify_weather(req);
    EXPECT_EQ(action.kind, WeatherActionKind::drop_wrong_user_level);
}

TEST(WeatherClassifierPlan, GmClassWithMasterOrEventerForwards) {
    WeatherRequest req{};
    req.protocol = weather_set;
    req.user_found = true;
    req.user_level = user_level_gm_weather;
    req.is_gm_master_or_eventer = true;
    req.map_num = 5u;
    const auto action = classify_weather(req);
    EXPECT_EQ(action.kind, WeatherActionKind::forward_to_map_by_data);
    EXPECT_EQ(action.target_map, 5u);
    const auto plan = weather_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, WeatherSideEffectKind::ForwardToMapByData);
    EXPECT_EQ(plan.effects[0].target_map, 5u);
}

TEST(WeatherClassifierPlan, ProgrammerForwards) {
    WeatherRequest req{};
    req.protocol = weather_exe;
    req.user_found = true;
    req.user_level = user_level_programmer;
    req.data_field = 13u;
    const auto action = classify_weather(req);
    EXPECT_EQ(action.kind, WeatherActionKind::forward_to_map_by_data);
    EXPECT_EQ(action.target_map, 13u);
}

TEST(WeatherClassifierPlan, DeveloperForwards) {
    WeatherRequest req{};
    req.protocol = weather_return;
    req.user_found = true;
    req.user_level = user_level_developer;
    req.data_field = 25u;
    const auto action = classify_weather(req);
    EXPECT_EQ(action.kind, WeatherActionKind::forward_to_map_by_data);
    EXPECT_EQ(action.target_map, 25u);
}

TEST(WeatherClassifierPlan, SetProtocolMapsToMapNum) {
    WeatherRequest req{};
    req.protocol = weather_set;
    req.user_found = true;
    req.user_level = user_level_developer;
    req.map_num = 0x1E2Du;
    const auto action = classify_weather(req);
    EXPECT_EQ(action.kind, WeatherActionKind::forward_to_map_by_data);
    EXPECT_EQ(action.target_map, 0x1E2Du & 0xFFFFu);
}

TEST(WeatherClassifierPlan, UnknownProtocolForwardsWithZeroTarget) {
    WeatherRequest req{};
    req.protocol = 99u;
    req.user_found = true;
    req.user_level = user_level_developer;
    const auto action = classify_weather(req);
    EXPECT_EQ(action.kind, WeatherActionKind::forward_to_map_by_data);
    EXPECT_EQ(action.target_map, 0u);
}