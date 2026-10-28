#include "mxh/server/player.hpp"
#include "mxh/server/money_manager.hpp"
#include "mxh/server/original_experience.hpp"

#include <gtest/gtest.h>
#include <limits>

namespace {
mxh::server::PlayerSpawnInfo spawn() {
    mxh::server::PlayerSpawnInfo info;
    info.player_id = 1001;
    info.user_id = 77;
    info.level = 10;
    info.map_num = 73;
    info.name = "Moxian";
    info.base.level = 10;
    info.base.cheryuk = 20;
    info.base.simmek = 10;
    return info;
}

mxh::server::Player active_player() {
    mxh::server::Player player;
    EXPECT_TRUE(player.initialize(spawn()));
    EXPECT_TRUE(player.activate());
    return player;
}
}

TEST(PlayerLifecycle, InitializeThenActivateMatchesMapEntryFlow) {
    mxh::server::Player player;
    EXPECT_EQ(player.lifecycle(), mxh::server::PlayerLifecycle::Disconnected);
    EXPECT_TRUE(player.initialize(spawn()));
    EXPECT_EQ(player.lifecycle(), mxh::server::PlayerLifecycle::Loading);
    EXPECT_TRUE(player.activate());
    EXPECT_TRUE(player.is_active());
    EXPECT_EQ(player.state().map_num, 73u);
    EXPECT_EQ(player.state().name, "Moxian");
}

TEST(PlayerLifecycle, ReinitializeWhileActiveIsRejected) {
    auto player = active_player();
    EXPECT_FALSE(player.initialize(spawn()));
    EXPECT_TRUE(player.begin_logout());
    player.release();
    EXPECT_TRUE(player.initialize(spawn()));
}

TEST(PlayerMoney, AddAndSpendRespectLegacyCap) {
    auto player = active_player();
    EXPECT_TRUE(player.set_money(mxh::server::MXH_PLAYER_MAX_MONEY - 5));
    EXPECT_TRUE(player.add_money(10));
    EXPECT_EQ(player.state().progress.money, mxh::server::MXH_PLAYER_MAX_MONEY);
    EXPECT_TRUE(player.spend_money(100));
    EXPECT_EQ(player.state().progress.money, mxh::server::MXH_PLAYER_MAX_MONEY - 100);
    EXPECT_TRUE(player.add_money(1));
    EXPECT_FALSE(player.spend_money(mxh::server::MXH_PLAYER_MAX_MONEY));
}

TEST(PlayerExperience, CarriesRemainderAcrossLevels) {
    auto player = active_player();
    player.state().progress.level_exp = 90;
    EXPECT_EQ(player.add_experience(25, 100), 1u);
    EXPECT_EQ(player.state().progress.level, 11u);
    EXPECT_EQ(player.state().progress.level_exp, 15u);
    EXPECT_EQ(player.state().progress.total_exp, 25u);
}

TEST(PlayerExperience, OneGrantRaisesAtMostOneLevelLikeLegacySetPlayerExpPoint) {
    auto player = active_player();
    EXPECT_EQ(player.add_experience(350, 100), 1u);
    EXPECT_EQ(player.state().progress.level, 11u);
    EXPECT_EQ(player.state().progress.level_exp, 250u);
    EXPECT_EQ(player.state().progress.total_exp, 350u);
    EXPECT_EQ(player.add_experience(1, 100), 1u);
    EXPECT_EQ(player.state().progress.level, 12u);
    EXPECT_EQ(player.state().progress.level_exp, 151u);
}

TEST(OriginalExperience, NormalSegmentsAwardOnceAndPersistRecoveryFlag) {
    mxh::server::PlayerProgress before;
    before.level = before.max_level = 1;
    before.exp_flag = 0x10u;
    const auto first = mxh::server::calculate_original_experience_award(before, 4, 20, 60);
    ASSERT_TRUE(first);
    EXPECT_FALSE(first->leveled);
    EXPECT_TRUE(first->refill_vitals);
    EXPECT_EQ(first->progress.level_up_points, 1u);
    EXPECT_EQ(first->progress.exp_flag, 0x11u);
    EXPECT_EQ(first->progress.hp_mp_flag, 1u);
    const auto second = mxh::server::calculate_original_experience_award(
        first->progress, 1, 20, 60);
    ASSERT_TRUE(second);
    EXPECT_FALSE(second->refill_vitals);
    EXPECT_EQ(second->progress.level_up_points, 1u);
}

TEST(OriginalExperience, LargeOneGrantRaisesOnlyOneLevelAndCarriesSegments) {
    mxh::server::PlayerProgress before;
    before.level = before.max_level = 1;
    before.exp_flag = 0x10u;
    const auto award = mxh::server::calculate_original_experience_award(before, 123, 20, 60);
    ASSERT_TRUE(award);
    EXPECT_TRUE(award->leveled);
    EXPECT_TRUE(award->refill_vitals);
    EXPECT_EQ(award->progress.level, 2u);
    EXPECT_EQ(award->progress.max_level, 2u);
    EXPECT_EQ(award->progress.level_exp, 103u);
    EXPECT_EQ(award->progress.level_up_points, 10u);
    EXPECT_EQ(award->progress.exp_flag, 0x1fu);
    EXPECT_EQ(award->progress.hp_mp_flag, 0u);
}

TEST(OriginalExperience, OverSeventyLevelAndPointOverflowAreBounded) {
    mxh::server::PlayerProgress before;
    before.level = before.max_level = 70;
    before.exp_flag = 0x1fu;
    const auto award = mxh::server::calculate_original_experience_award(before, 100, 100, 1000);
    ASSERT_TRUE(award);
    EXPECT_EQ(award->progress.level, 71u);
    EXPECT_EQ(award->progress.level_up_points, 4u);
    before.level_up_points = 32767;
    EXPECT_FALSE(mxh::server::calculate_original_experience_award(before, 100, 100, 1000));
}

TEST(OriginalExperience, RejectsNonFiniteLiveEventAwardBeforePublishingKill) {
    using mxh::server::bounded_original_experience_amount;
    EXPECT_EQ(bounded_original_experience_amount(123u, 1.0), 123u);
    EXPECT_EQ(bounded_original_experience_amount(123u, 2.0), 246u);
    EXPECT_FALSE(bounded_original_experience_amount(
        123u, std::numeric_limits<double>::quiet_NaN()));
    EXPECT_FALSE(bounded_original_experience_amount(
        123u, std::numeric_limits<double>::infinity()));
    EXPECT_FALSE(bounded_original_experience_amount(123u, -1.0));
    EXPECT_EQ(bounded_original_experience_amount(
        std::numeric_limits<std::uint32_t>::max(), 10.0),
        std::numeric_limits<std::uint32_t>::max());
}

TEST(PlayerInventory, RequestedEmptyPositionWins) {
    auto player = active_player();
    auto item = mxh::game::make_item(500, 42, 5);
    auto slot = player.insert_inventory_item(item);
    ASSERT_TRUE(slot.has_value());
    EXPECT_EQ(*slot, 5u);
    EXPECT_EQ(player.state().inventory.items[5].Position, 5u);
}

TEST(PlayerInventory, OccupiedRequestedPositionFallsBackToFirstEmpty) {
    auto player = active_player();
    player.state().inventory.items[5] = mxh::game::make_item(1, 1, 5);
    auto slot = player.insert_inventory_item(mxh::game::make_item(2, 2, 5));
    ASSERT_TRUE(slot.has_value());
    EXPECT_EQ(*slot, 0u);
}

TEST(PlayerInventory, DuplicateDbItemIsRejected) {
    auto player = active_player();
    EXPECT_TRUE(player.insert_inventory_item(mxh::game::make_item(500, 42, 0)));
    EXPECT_FALSE(player.insert_inventory_item(mxh::game::make_item(500, 42, 1)));
}

TEST(PlayerEquipment, EquipAndUnequipMoveWirePositions) {
    auto player = active_player();
    ASSERT_TRUE(player.insert_inventory_item(mxh::game::make_item(500, 42, 0)));
    EXPECT_TRUE(player.equip_inventory_item(0, mxh::game::WEARED_WEAPON));
    EXPECT_EQ(player.state().equipment.items[mxh::game::WEARED_WEAPON].Position, 81u);
    EXPECT_TRUE(player.unequip_item(mxh::game::WEARED_WEAPON, 3));
    EXPECT_EQ(player.state().inventory.items[3].Position, 3u);
    EXPECT_EQ(player.state().equipment.items[mxh::game::WEARED_WEAPON].dwDBIdx, 0u);
}

TEST(PlayerDamage, NormalShieldAbsorbsBeforeLife) {
    auto player = active_player();
    player.state().vitals.current_shield = 50;
    player.state().vitals.current_hp = 100;
    const auto result = player.apply_damage(80);
    EXPECT_EQ(result.shield_damage, 50u);
    EXPECT_EQ(result.life_damage, 30u);
    EXPECT_EQ(player.state().vitals.current_hp, 70u);
    EXPECT_FALSE(result.died);
}

TEST(PlayerDamage, MussangUsesSeventyPercentShieldReduction) {
    auto player = active_player();
    player.state().mussang_mode = true;
    player.state().vitals.current_shield = 100;
    player.state().vitals.current_hp = 100;
    const auto result = player.apply_damage(100);
    EXPECT_EQ(result.shield_damage, 70u);
    EXPECT_EQ(result.life_damage, 0u);
    EXPECT_EQ(player.state().vitals.current_hp, 100u);
}

TEST(PlayerDamage, DeathAndReviveTransition) {
    auto player = active_player();
    player.state().vitals.current_shield = 0;
    player.state().vitals.current_hp = 10;
    const auto result = player.apply_damage(10);
    EXPECT_TRUE(result.died);
    EXPECT_EQ(player.lifecycle(), mxh::server::PlayerLifecycle::Dead);
    EXPECT_TRUE(player.revive());
    EXPECT_EQ(player.lifecycle(), mxh::server::PlayerLifecycle::Active);
    EXPECT_EQ(player.state().vitals.current_hp, player.state().vitals.max_hp / 2);
}

TEST(PlayerRecovery, HealFullRestoresAllVitals) {
    auto player = active_player();
    player.state().vitals.current_hp = 1;
    player.state().vitals.current_shield = 0;
    player.state().vitals.current_mp = 0;
    player.heal_full();
    EXPECT_EQ(player.state().vitals.current_hp, player.state().vitals.max_hp);
    EXPECT_EQ(player.state().vitals.current_shield, player.state().vitals.max_shield);
    EXPECT_EQ(player.state().vitals.current_mp, player.state().vitals.max_mp);
}




