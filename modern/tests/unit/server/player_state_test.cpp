// player_state_test.cpp - unit tests for PlayerState struct + helpers.

#include "mxh/server/player_state.hpp"
#include "mxh/game/item_types.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::PlayerState;
using mxh::server::PlayerVitals;
using mxh::server::PlayerProgress;
using mxh::server::SkillBook;
using mxh::server::LearnedSkill;
using mxh::server::InventorySlots;
using mxh::server::QuickBar;
using mxh::server::GuildMembership;
using mxh::server::PartyMembership;
using mxh::server::make_player_state;
using mxh::server::apply_hp_delta;
using mxh::server::apply_shield_delta;
using mxh::server::apply_mp_delta;
using mxh::server::add_learned_skill;
using mxh::server::find_learned_skill;
using mxh::server::remove_learned_skill;
using mxh::server::find_inventory_slot;
using mxh::server::inventory_occupied_count;
using mxh::server::is_inventory_slot_empty;
using mxh::server::find_quick_slot_binding;
using mxh::server::is_in_guild;
using mxh::server::is_in_party;
using mxh::server::CalcBaseStats;
using mxh::server::CalcEquipBonuses;
using mxh::game::ItemBase;
}

// ---- make_player_state ----
TEST(PlayerStateMake, BasicSpawnFullVitals) {
    CalcBaseStats b; b.level = 20; b.cheryuk = 50;
    auto s = make_player_state(1001, 7, 20, b, CalcEquipBonuses{});
    EXPECT_EQ(s.player_id, 1001u);
    EXPECT_EQ(s.user_id, 7u);
    EXPECT_EQ(s.progress.level, 20u);
    // max_hp = 20*5 + 50*10 = 600
    EXPECT_EQ(s.vitals.max_hp, 600u);
    EXPECT_EQ(s.vitals.current_hp, 600u);  // spawned full
}

TEST(PlayerStateMake, WithBonusesMaxHpIncreases) {
    CalcBaseStats b; b.level = 30; b.cheryuk = 50;
    CalcEquipBonuses bonuses; bonuses.item_max_life = 100; bonuses.shop_life = 50;
    auto s = make_player_state(1, 1, 30, b, bonuses);
    EXPECT_EQ(s.vitals.max_hp, 800u);  // base 650 + 100 + 50
    EXPECT_EQ(s.vitals.current_hp, 800u);
}

TEST(PlayerStateMake, MaxShieldSpawn) {
    CalcBaseStats b; b.level = 10; b.simmek = 30; b.gengol = 10; b.minchub = 5;
    auto s = make_player_state(1, 1, 10, b, CalcEquipBonuses{});
    // 10*5 + 30*10 + 10*5 + 5*5 = 50 + 300 + 50 + 25 = 425
    EXPECT_EQ(s.vitals.max_shield, 425u);
    EXPECT_EQ(s.vitals.current_shield, 425u);
}

TEST(PlayerStateMake, MaxNaeRyukSpawn) {
    CalcBaseStats b; b.level = 10; b.simmek = 30;
    auto s = make_player_state(1, 1, 10, b, CalcEquipBonuses{});
    // 10*5 + 30*10 = 350
    EXPECT_EQ(s.vitals.max_mp, 350u);
    EXPECT_EQ(s.vitals.current_mp, 350u);
}

// ---- recompute_max_stats ----
TEST(PlayerStateRecompute, AfterAttributeChange) {
    CalcBaseStats b; b.level = 1; b.cheryuk = 10;
    auto s = make_player_state(1, 1, 1, b, CalcEquipBonuses{});
    EXPECT_EQ(s.vitals.max_hp, 105u);
    s.attributes.cheryuk = 100;  // +90*10 = +900 hp
    s.recompute_max_stats();
    EXPECT_EQ(s.vitals.max_hp, 1005u);  // 5 + 1000 = 1005
}

// ---- apply_hp_delta ----
TEST(ApplyHpDelta, DamageReducesHp) {
    PlayerVitals v; v.max_hp = 1000; v.current_hp = 1000;
    auto applied = apply_hp_delta(v, -300);
    EXPECT_EQ(applied, -300);
    EXPECT_EQ(v.current_hp, 700u);
}

TEST(ApplyHpDelta, OverdamageClampsToZero) {
    PlayerVitals v; v.max_hp = 100; v.current_hp = 50;
    auto applied = apply_hp_delta(v, -100);
    EXPECT_EQ(applied, -50);
    EXPECT_EQ(v.current_hp, 0u);
}

TEST(ApplyHpDelta, HealClampsToMax) {
    PlayerVitals v; v.max_hp = 100; v.current_hp = 50;
    auto applied = apply_hp_delta(v, 200);
    EXPECT_EQ(applied, 50);
    EXPECT_EQ(v.current_hp, 100u);
}

TEST(ApplyShieldDelta, Basic) {
    PlayerVitals v; v.max_shield = 500; v.current_shield = 500;
    EXPECT_EQ(apply_shield_delta(v, -100), -100);
    EXPECT_EQ(v.current_shield, 400u);
}

TEST(ApplyMpDelta, Basic) {
    PlayerVitals v; v.max_mp = 200; v.current_mp = 200;
    EXPECT_EQ(apply_mp_delta(v, -50), -50);
    EXPECT_EQ(v.current_mp, 150u);
}

// ---- SkillBook ----
TEST(SkillBook, AddLearnedSkill) {
    SkillBook b;
    EXPECT_TRUE(add_learned_skill(b, 1001, 1));
    EXPECT_EQ(b.count, 1u);
    EXPECT_EQ(b.skills[0].mugong_idx, 1001u);
    EXPECT_EQ(b.skills[0].level, 1u);
}

TEST(SkillBook, AddDuplicateReturnsFalse) {
    SkillBook b;
    EXPECT_TRUE(add_learned_skill(b, 1001, 1));
    EXPECT_FALSE(add_learned_skill(b, 1001, 2));  // already learned
    EXPECT_EQ(b.count, 1u);
}

TEST(SkillBook, FindLearnedSkill) {
    SkillBook b;
    add_learned_skill(b, 1001, 3);
    add_learned_skill(b, 2002, 5);
    auto found = find_learned_skill(b, 2002);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->level, 5u);
}

TEST(SkillBook, FindNotLearnedIsEmpty) {
    SkillBook b;
    add_learned_skill(b, 1001, 1);
    EXPECT_FALSE(find_learned_skill(b, 9999).has_value());
}

TEST(SkillBook, RemoveLearnedSkillShifts) {
    SkillBook b;
    add_learned_skill(b, 1001, 1);
    add_learned_skill(b, 2002, 2);
    add_learned_skill(b, 3003, 3);
    EXPECT_TRUE(remove_learned_skill(b, 2002));
    EXPECT_EQ(b.count, 2u);
    EXPECT_EQ(b.skills[0].mugong_idx, 1001u);
    EXPECT_EQ(b.skills[1].mugong_idx, 3003u);  // shift
}

TEST(SkillBook, RemoveUnknownReturnsFalse) {
    SkillBook b;
    add_learned_skill(b, 1001, 1);
    EXPECT_FALSE(remove_learned_skill(b, 9999));
    EXPECT_EQ(b.count, 1u);
}

// ---- Inventory ----
TEST(Inventory, EmptyByDefault) {
    InventorySlots inv;
    EXPECT_EQ(inventory_occupied_count(inv), 0u);
    EXPECT_TRUE(is_inventory_slot_empty(inv, 0));
}

TEST(Inventory, FindByIconIdx) {
    InventorySlots inv;
    inv.items[5].dwDBIdx = 100;
    inv.items[5].wIconIdx = 42;
    auto pos = find_inventory_slot(inv, 42);
    ASSERT_TRUE(pos.has_value());
    EXPECT_EQ(*pos, 5u);
    EXPECT_EQ(inventory_occupied_count(inv), 1u);
}

TEST(Inventory, FindNotPresent) {
    InventorySlots inv;
    EXPECT_FALSE(find_inventory_slot(inv, 99).has_value());
}

TEST(Inventory, CountAfterMultipleInserts) {
    InventorySlots inv;
    inv.items[0].dwDBIdx = 1; inv.items[0].wIconIdx = 10;
    inv.items[10].dwDBIdx = 2; inv.items[10].wIconIdx = 20;
    inv.items[79].dwDBIdx = 3; inv.items[79].wIconIdx = 30;
    EXPECT_EQ(inventory_occupied_count(inv), 3u);
    EXPECT_FALSE(is_inventory_slot_empty(inv, 0));
    EXPECT_TRUE(is_inventory_slot_empty(inv, 1));
}

// ---- QuickBar ----
TEST(QuickBar, EmptyBindingIsNullopt) {
    QuickBar bar;
    EXPECT_FALSE(find_quick_slot_binding(bar, 1001).has_value());
}

TEST(QuickBar, BindingFoundAcrossSheets) {
    QuickBar bar;
    bar.sheets[2][3].skill_idx = 5005;
    auto found = find_quick_slot_binding(bar, 5005);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, 0x23);  // sheet 2, item 3
}

TEST(QuickBar, SkillIdxZeroNeverBinds) {
    QuickBar bar;
    bar.sheets[0][0].skill_idx = 0;
    EXPECT_FALSE(find_quick_slot_binding(bar, 0).has_value());
}

// ---- Membership ----
TEST(Membership, GuildZeroIsNotMember) {
    GuildMembership g;
    EXPECT_FALSE(is_in_guild(g));
    g.guild_id = 42;
    EXPECT_TRUE(is_in_guild(g));
}

TEST(Membership, PartyZeroIsNotMember) {
    PartyMembership p;
    EXPECT_FALSE(is_in_party(p));
    p.party_id = 99;
    EXPECT_TRUE(is_in_party(p));
}

// ---- PlayerState composition ----
TEST(PlayerStateComposition, FullPlayerInitializedFromFactory) {
    CalcBaseStats b; b.level = 25; b.cheryuk = 50; b.simmek = 30;
    auto s = make_player_state(7, 1, 25, b, CalcEquipBonuses{});
    EXPECT_EQ(s.player_id, 7u);
    EXPECT_EQ(s.vitals.max_hp, 625u);  // 25*5 + 50*10 = 625
    EXPECT_EQ(s.vitals.max_shield, 25u * 5 + 30u * 10);  // 425
    EXPECT_EQ(s.vitals.max_mp, 25u * 5 + 30u * 10);  // 425
    EXPECT_EQ(s.inventory.items.size(), 80u);
    EXPECT_EQ(s.equipment.items.size(), 10u);
    EXPECT_EQ(s.pyoguk.items.size(), 80u);
}

TEST(PlayerStateComposition, AddSkillAndFindInQuickBar) {
    CalcBaseStats b; b.level = 10;
    auto s = make_player_state(1, 1, 10, b, CalcEquipBonuses{});
    add_learned_skill(s.skills, 100, 1);
    s.quick.sheets[0][0].skill_idx = 100;
    EXPECT_TRUE(find_learned_skill(s.skills, 100).has_value());
    EXPECT_TRUE(find_quick_slot_binding(s.quick, 100).has_value());
}

// ---- Recovery snapshot ----
TEST(PlayerVitals, RecoverySnapshotInit) {
    PlayerVitals v; v.max_hp = 100; v.current_hp = 100;
    EXPECT_FALSE(v.life_recover_active);
    EXPECT_EQ(v.pending_life_count, 0u);
    EXPECT_EQ(v.last_life_check_ms, 0u);
}

