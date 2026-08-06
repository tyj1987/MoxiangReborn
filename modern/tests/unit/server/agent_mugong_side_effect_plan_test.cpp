// D4.117 -- AgentMugong side-effect plan unit tests.
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_MUGONG.
//

#include <gtest/gtest.h>

#include "mxh/server/agent_mugong.hpp"
#include "mxh/server/agent_mugong_side_effect_plan.hpp"

using namespace mxh::server;

TEST(MugongPlan, ForwardToMapEmitsRawForward) {
    MugongAction a{};
    a.kind = MugongActionKind::forward_to_map;
    a.protocol = mugong_totalinfo_local;
    a.object_id = 11u;
    const auto plan = mugong_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, MugongSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].reply_protocol, mugong_totalinfo_local);
    EXPECT_TRUE(mugong_effect_targets_map(plan.effects[0]));
}

TEST(MugongPlan, SendNackEmitsOptionNackEffect) {
    MugongAction a{};
    a.kind = MugongActionKind::send_nack;
    a.protocol = mugong_option_nack;
    a.object_id = 11u;
    a.error_code = 1u;
    const auto plan = mugong_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, MugongSideEffectKind::SendOptionNackToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, mugong_option_nack);
    EXPECT_EQ(plan.effects[0].error_code, 1u);
    EXPECT_TRUE(mugong_effect_targets_user(plan.effects[0]));
}

TEST(MugongPlan, ForwardToMapIfLevelOkAlsoEmitsForward) {
    MugongAction a{};
    a.kind = MugongActionKind::forward_to_map_if_level_ok;
    a.protocol = mugong_option_syn;
    const auto plan = mugong_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, MugongSideEffectKind::ForwardRawToMap);
}

// Classifier 1:1

TEST(MugongClassifierPlan, OptionSynLowLevelEmitsNackPlan) {
    MugongRequest req{};
    req.protocol = mugong_option_syn;
    req.required_level = 50u;
    req.mugong_level = 30u;
    const auto action = classify_mugong(req);
    EXPECT_EQ(action.kind, MugongActionKind::send_nack);
    EXPECT_EQ(action.protocol, mugong_option_nack);
    EXPECT_EQ(action.error_code, 1u);
    const auto plan = mugong_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, MugongSideEffectKind::SendOptionNackToUser);
    EXPECT_EQ(plan.effects[0].error_code, 1u);
}

TEST(MugongClassifierPlan, OptionSynHighLevelForwardsToMap) {
    MugongRequest req{};
    req.protocol = mugong_option_syn;
    req.required_level = 50u;
    req.mugong_level = 60u;
    const auto action = classify_mugong(req);
    EXPECT_EQ(action.kind, MugongActionKind::forward_to_map);
    EXPECT_EQ(action.protocol, mugong_option_syn);
    const auto plan = mugong_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, MugongSideEffectKind::ForwardRawToMap);
}

TEST(MugongClassifierPlan, OptionSynZeroRequiredLevelForwards) {
    MugongRequest req{};
    req.protocol = mugong_option_syn;
    req.required_level = 0u;
    req.mugong_level = 1u;
    const auto action = classify_mugong(req);
    EXPECT_EQ(action.kind, MugongActionKind::forward_to_map);
}

TEST(MugongClassifierPlan, AllNonOptionProtocolsForward) {
    for (auto proto : {mugong_totalinfo_local, mugong_move_syn, mugong_rem_syn, mugong_add_syn, mugong_deletegroundadd_syn, mugong_deleteinventoryadd_syn, mugong_exppoint_notify, mugong_sung_notify, mugong_sung_levelup, mugong_option_clear_syn}) {
        MugongRequest req{};
        req.protocol = proto;
        const auto action = classify_mugong(req);
        EXPECT_EQ(action.kind, MugongActionKind::forward_to_map);
        EXPECT_EQ(action.protocol, proto);
        const auto plan = mugong_side_effect_plan(action);
        EXPECT_EQ(plan.effects[0].kind, MugongSideEffectKind::ForwardRawToMap);
    }
}