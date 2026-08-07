//
// 1:1 lock the implicit default-branch dispatch for MP_MP_CHAR in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_char.hpp"
#include "mxh/server/agent_char_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentCharPlan, UserFoundEmitsForwardEffect) {
    AgentCharRequest r;
    r.protocol = mp_char_life_syn;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_char_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentCharSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, mp_char_life_syn);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(mp_char_effect_targets_user(plan.effects[0]));
}

TEST(AgentCharPlan, UserMissingEmitsDropEffect) {
    AgentCharRequest r;
    r.protocol = mp_char_life_syn;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_char_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentCharSideEffectKind::Drop);
    EXPECT_FALSE(mp_char_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentCharPlan, ForwardPlanPreservesObjectId) {
    AgentCharRequest r;
    r.protocol = mp_char_life_syn;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_char_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentCharPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        mp_char_life_syn,
        mp_char_life_ack,
        mp_char_life_nack,
        mp_char_maxlife_notify,
        mp_char_shield_syn,
        mp_char_shield_ack,
        mp_char_shield_nack,
        mp_char_maxshield_notify,
        mp_char_naeryuk_syn,
        mp_char_naeryuk_ack,
        mp_char_naeryuk_nack,
        mp_char_maxnaeryuk_notify,
        mp_char_exppoint_syn,
        mp_char_exppoint_ack,
        mp_char_exppoint_nack,
        mp_char_gengol_notify,
        mp_char_minchub_notify,
        mp_char_simmek_notify,
        mp_char_cheryuk_notify,
        mp_char_level_notify,
        mp_char_playerlevelup_notify,
        mp_char_pointadd_syn,
        mp_char_pointadd_ack,
        mp_char_pointadd_nack,
        mp_char_leveluppoint_notify,
        mp_char_leveldown_syn,
        mp_char_leveldown_ack,
        mp_char_leveldown_nack,
        mp_char_fame_notify,
        mp_char_state_notify,
        mp_char_life_notify,
        mp_char_abilityexppoint_syn,
        mp_char_abilityexppoint_ack,
        mp_char_abilityexppoint_nack,
        mp_char_ability_upgrade_syn,
        mp_char_ability_upgrade_ack,
        mp_char_ability_upgrade_nack,
        mp_char_youaredied,
        mp_char_exitstart_syn,
        mp_char_exitstart_ack,
        mp_char_exitstart_nack,
        mp_char_exit_syn,
        mp_char_exit_ack,
        mp_char_exit_nack,
        mp_char_badfame_notify,
        mp_char_badfame_syn,
        mp_char_badfame_ack,
        mp_char_badfame_nack,
        mp_char_badfame_changed,
        mp_char_playtime_syn,
        mp_char_playtime_ack,
        mp_char_playtime_nack,
        mp_char_pointminus_syn,
        mp_char_pointminus_ack,
        mp_char_pointminus_nack,
        mp_char_ability_upgrade_skpoint_syn,
        mp_char_ability_upgrade_skpoint_ack,
        mp_char_ability_upgrade_skpoint_nack,
        mp_char_ability_downgrade_skpoint_syn,
        mp_char_ability_downgrade_skpoint_ack,
        mp_char_ability_downgrade_skpoint_nack,
        mp_char_stage_notify,
        mp_char_change_subattr_ack,
        mp_char_change_subattr_nack,
        mp_char_mussang_syn,
        mp_char_mussang_ack,
        mp_char_mussang_nack,
        mp_char_mussang_info,
        mp_char_mussang_end,
        mp_char_single_special_state_notify,
        mp_char_single_special_state_ack,
        mp_char_single_special_state_nack,
        mp_char_fullmoonevent_change,
        mp_char_noactionpanelty_notify,
        mp_char_ability_reset_skpoint_syn,
        mp_char_ability_reset_skpoint_ack,
        mp_char_ability_reset_skpoint_nack
    };
    for (std::uint8_t p : all) {
        AgentCharRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_char_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentCharPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        mp_char_life_syn,
        mp_char_life_ack,
        mp_char_life_nack,
        mp_char_maxlife_notify,
        mp_char_shield_syn,
        mp_char_shield_ack,
        mp_char_shield_nack,
        mp_char_maxshield_notify,
        mp_char_naeryuk_syn,
        mp_char_naeryuk_ack,
        mp_char_naeryuk_nack,
        mp_char_maxnaeryuk_notify,
        mp_char_exppoint_syn,
        mp_char_exppoint_ack,
        mp_char_exppoint_nack,
        mp_char_gengol_notify,
        mp_char_minchub_notify,
        mp_char_simmek_notify,
        mp_char_cheryuk_notify,
        mp_char_level_notify,
        mp_char_playerlevelup_notify,
        mp_char_pointadd_syn,
        mp_char_pointadd_ack,
        mp_char_pointadd_nack,
        mp_char_leveluppoint_notify,
        mp_char_leveldown_syn,
        mp_char_leveldown_ack,
        mp_char_leveldown_nack,
        mp_char_fame_notify,
        mp_char_state_notify,
        mp_char_life_notify,
        mp_char_abilityexppoint_syn,
        mp_char_abilityexppoint_ack,
        mp_char_abilityexppoint_nack,
        mp_char_ability_upgrade_syn,
        mp_char_ability_upgrade_ack,
        mp_char_ability_upgrade_nack,
        mp_char_youaredied,
        mp_char_exitstart_syn,
        mp_char_exitstart_ack,
        mp_char_exitstart_nack,
        mp_char_exit_syn,
        mp_char_exit_ack,
        mp_char_exit_nack,
        mp_char_badfame_notify,
        mp_char_badfame_syn,
        mp_char_badfame_ack,
        mp_char_badfame_nack,
        mp_char_badfame_changed,
        mp_char_playtime_syn,
        mp_char_playtime_ack,
        mp_char_playtime_nack,
        mp_char_pointminus_syn,
        mp_char_pointminus_ack,
        mp_char_pointminus_nack,
        mp_char_ability_upgrade_skpoint_syn,
        mp_char_ability_upgrade_skpoint_ack,
        mp_char_ability_upgrade_skpoint_nack,
        mp_char_ability_downgrade_skpoint_syn,
        mp_char_ability_downgrade_skpoint_ack,
        mp_char_ability_downgrade_skpoint_nack,
        mp_char_stage_notify,
        mp_char_change_subattr_ack,
        mp_char_change_subattr_nack,
        mp_char_mussang_syn,
        mp_char_mussang_ack,
        mp_char_mussang_nack,
        mp_char_mussang_info,
        mp_char_mussang_end,
        mp_char_single_special_state_notify,
        mp_char_single_special_state_ack,
        mp_char_single_special_state_nack,
        mp_char_fullmoonevent_change,
        mp_char_noactionpanelty_notify,
        mp_char_ability_reset_skpoint_syn,
        mp_char_ability_reset_skpoint_ack,
        mp_char_ability_reset_skpoint_nack
    };
    for (std::uint8_t p : all) {
        AgentCharRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_char_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentCharSideEffectKind::Drop);
    }
}

TEST(AgentCharPlan, DefaultPlanStructFieldsAreStable) {
    AgentCharSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentCharPlan, EffectTargetsUserPredicateMatchesForward) {
    AgentCharSideEffect forward{};
    forward.kind = AgentCharSideEffectKind::ForwardToUser;
    AgentCharSideEffect drop{};
    drop.kind = AgentCharSideEffectKind::Drop;
    EXPECT_TRUE(mp_char_effect_targets_user(forward));
    EXPECT_FALSE(mp_char_effect_targets_user(drop));
}

TEST(AgentCharPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentCharRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_char_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentCharPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentCharRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_char_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
