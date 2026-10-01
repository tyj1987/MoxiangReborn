#include "synthetic_server_resources.hpp"
#include "mxh/game/login_point.hpp"
#include "mxh/game/exp_penalty.hpp"
#include "mxh/server/ai_group_loader.hpp"
#include <gtest/gtest.h>
#include <set>

namespace fixture = mxh::test::resources;
TEST(SyntheticServerResources, LoginPointProfilesPreserveFirstPointAndCoordinateBoundaries) {
    for (const bool current : {false, true}) {
        const auto bytes = current ? fixture::current(fixture::login_text, fixture::login_key)
                                   : fixture::legacy(fixture::login_text);
        const auto table = mxh::game::decode_login_points(bytes, current
            ? mxh::game::LoginPointProfile::PlayDhCurrent : mxh::game::LoginPointProfile::LegacyMhFile);
        ASSERT_TRUE(table); ASSERT_EQ(table->size(), 3u);
        EXPECT_FLOAT_EQ(table->at(10)[0].x, 12345);
        EXPECT_FLOAT_EQ(table->at(10)[0].z, 23456);
        ASSERT_EQ(table->at(12).size(), 2u);
        EXPECT_FLOAT_EQ(mxh::game::login_revive_point(*table, 12)->x, 65535);
        EXPECT_FLOAT_EQ(table->at(12)[1].x, 0);
        EXPECT_FLOAT_EQ(table->at(12)[1].z, 65535);
        EXPECT_FALSE(mxh::game::login_revive_point(*table, 999));
    }
    const auto invalid = fixture::legacy("1 Bad 10 1 65536 1 0\n");
    EXPECT_FALSE(mxh::game::decode_login_points(invalid, mxh::game::LoginPointProfile::LegacyMhFile));
}
TEST(SyntheticServerResources, PenaltyProfilesPreserveValuesAndFallback) {
    for (const bool current : {false, true}) {
        const auto bytes = current ? fixture::current(fixture::penalty_text, fixture::penalty_key)
                                   : fixture::legacy(fixture::penalty_text);
        const auto table = mxh::game::decode_exp_penalty(bytes, current
            ? mxh::game::ExpPenaltyProfile::PlayDhCurrent : mxh::game::ExpPenaltyProfile::LegacyMhFile);
        ASSERT_TRUE(table); ASSERT_EQ(table->size(), 4u);
        EXPECT_FLOAT_EQ(table->at(48).present_percent, 2.4f);
        EXPECT_FLOAT_EQ(table->at(48).login_percent, 1.9f);
        EXPECT_FLOAT_EQ(table->at(5).login_percent, 0);
        EXPECT_EQ(table->count(6), 0u);
        const auto fallback = mxh::game::unprotected_revive_loss(*table,
            mxh::game::ReviveLocation::Login, 6, 100000, 10000);
        ASSERT_TRUE(fallback); EXPECT_EQ(fallback->money, 4000u); EXPECT_EQ(fallback->experience, 200u);
    }
}
TEST(SyntheticServerResources, CorruptOrWrongProfileContainersAreRejected) {
    auto legacy = fixture::legacy(fixture::penalty_text);
    EXPECT_FALSE(mxh::game::decode_exp_penalty(legacy, mxh::game::ExpPenaltyProfile::PlayDhCurrent));
    legacy.back() ^= 1;
    EXPECT_FALSE(mxh::game::decode_exp_penalty(legacy, mxh::game::ExpPenaltyProfile::LegacyMhFile));
    auto current = fixture::current(fixture::penalty_text, fixture::penalty_key);
    EXPECT_FALSE(mxh::game::decode_exp_penalty(current, mxh::game::ExpPenaltyProfile::LegacyMhFile));
    current.pop_back();
    EXPECT_FALSE(mxh::game::decode_exp_penalty(current, mxh::game::ExpPenaltyProfile::PlayDhCurrent));
    auto login = fixture::legacy(fixture::login_text);
    login[12] ^= 1;
    EXPECT_FALSE(mxh::game::decode_login_points(login, mxh::game::LoginPointProfile::LegacyMhFile));
}
TEST(SyntheticServerResources, MonsterFileExercisesAll228SpawnsAcrossThreeGroups) {
    const fixture::TemporaryBin file(fixture::legacy(fixture::monster_text()));
    const auto groups = mxh::server::load_ai_group_list_bin(file.path, "sworking-2008-reference");
    ASSERT_TRUE(groups); ASSERT_EQ(groups->groups.size(), 3u);
    EXPECT_EQ(groups->spawn_count(), 228u);
    std::set<std::uint32_t> ids;
    for (const auto& group : groups->groups) {
        ASSERT_EQ(group.spawns.size(), 76u);
        EXPECT_EQ(group.max_object, 76u);
        for (const auto& spawn : group.spawns) {
            EXPECT_EQ(spawn.monster_kind, 100u + group.group_id);
            EXPECT_EQ(spawn.object_kind, 32u);
            EXPECT_FALSE(spawn.initially_dead);
            EXPECT_GE(spawn.pos_x, 1000); EXPECT_LE(spawn.pos_x, 1075);
            EXPECT_FLOAT_EQ(spawn.pos_z, 2000 + group.group_id);
            ids.insert(spawn.source_object_id);
        }
    }
    EXPECT_EQ(ids.size(), 228u);
}
