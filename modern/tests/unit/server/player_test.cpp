#include "mxh/server/player.hpp"
#include "mxh/server/money_manager.hpp"

#include <gtest/gtest.h>

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




