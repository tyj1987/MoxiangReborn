// D4 AgentChar data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_MP_CHAR.

#include <mxh/server/agent_char.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentCharClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(mp_char_category, 3u);
}

TEST(AgentCharClassify, SubProtocolConstantsAreUnique) {
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
    ASSERT_EQ(sizeof(all) / sizeof(all[0]), 77u);
    for (std::size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i) {
        for (std::size_t j = i + 1; j < sizeof(all) / sizeof(all[0]); ++j) {
            EXPECT_NE(all[i], all[j])
                << "duplicate protocol at i=" << i
                << " j=" << j;
        }
    }
}

TEST(AgentCharClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(mp_char_life_syn, 0u);
    EXPECT_EQ(mp_char_ability_reset_skpoint_nack, 76u);
}

TEST(AgentCharClassify, UserFoundForwards) {
    AgentCharRequest r;
    r.protocol = mp_char_life_syn;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_char(r), AgentCharOutcome::ForwardToUser);
}

TEST(AgentCharClassify, UserNotFoundDrops) {
    AgentCharRequest r;
    r.protocol = mp_char_life_syn;
    r.user_found = false;
    EXPECT_EQ(classify_agent_char(r), AgentCharOutcome::DropNoUser);
}

TEST(AgentCharClassify, EverySubProtocolForwardsWhenUserFound) {
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
        EXPECT_EQ(classify_agent_char(r), AgentCharOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentCharClassify, EverySubProtocolDropsWhenUserMissing) {
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
        EXPECT_EQ(classify_agent_char(r), AgentCharOutcome::DropNoUser)
            << "protocol=" << +p;
    }

    if (sizeof(all) / sizeof(all[0]) >= 2) {
        const std::uint8_t middle = all[sizeof(all) / sizeof(all[0]) / 2];
        AgentCharRequest m;
        m.protocol = middle;
        m.user_found = true;
        EXPECT_EQ(classify_agent_char(m), AgentCharOutcome::ForwardToUser);
    }
}

TEST(AgentCharClassify, ObjectIdIgnoredInClassification) {
    AgentCharRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentCharRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_char(a), classify_agent_char(b));
}

TEST(AgentCharClassify, OutcomeIsDeterministic) {
    AgentCharRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_char(r), AgentCharOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_char(r), AgentCharOutcome::ForwardToUser);
}

TEST(AgentCharClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentCharRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_char(r), AgentCharOutcome::ForwardToUser);
}
