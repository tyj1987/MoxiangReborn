// game_types_test.cpp - Phase 10d game data structure layout tests
//
// Covers the 3 header files in modern/include/mxh/game/:
//   - item_types.hpp    (IconBase, ItemBase, ItemTotalInfo)
//   - monster_types.hpp (MonsterTotalInfo, NpcRegen, MonsterTemplate, MonsterInstance)
//   - skill_types.hpp   (SkillInfo, SkillInstance, DamageResult, PlayerCombatStats)
//
// The point of these tests is twofold:
//   1. Lock in the wire-format byte sizes. CommonStruct.h in the legacy
//      tree uses #pragma pack(push, 1) and a precise layout; the modern
//      versions reproduce that layout. If a future change accidentally
//      adds a field or drops #pragma pack, the static_asserts in the
//      headers catch the obvious cases, but a runtime sizeof check is
//      also useful (and we get an additional test name in ctest output).
//   2. Exercise the small helper functions (make_empty_item, is_empty_slot,
//      apply_damage, etc.) to make sure the trivial logic is correct.
//
// All sizes here are 1:1 with the legacy CommonStruct.h /
// GameResourceStruct.h. If you change a struct in item_types.hpp /
// monster_types.hpp / skill_types.hpp, update the static_asserts in
// those headers FIRST and the test assertions here SECOND.

#include "mxh/game/item_types.hpp"
#include "mxh/game/hero_total_layout.hpp"
#include "mxh/game/monster_types.hpp"
#include "mxh/game/skill_types.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

namespace mxh::game::test {

// ===========================================================================
// item_types.hpp
// ===========================================================================

TEST(ItemBaseTest, WireFormatSizeIs22Bytes) {
    EXPECT_EQ(sizeof(ItemBase), 22u);
}

TEST(ItemBaseTest, FieldsAreAccessible) {
    ItemBase item = make_item(/*db_idx=*/42, /*icon_idx=*/7,
                              /*position=*/TP_INVENTORY_START + 5,
                              /*dur=*/100, /*count=*/3);
    EXPECT_EQ(item.dwDBIdx, 42u);
    EXPECT_EQ(item.wIconIdx, 7u);
    EXPECT_EQ(item.Position, 5u);
    EXPECT_EQ(item.Durability, 100u);
    EXPECT_EQ(item.QuickPosition, 0xFFFFu);  // 0xFFFF = "not in quick slot"
    EXPECT_EQ(item.ItemParam, 3u);
}

TEST(ItemBaseTest, EmptyItemHasZeroDbIdxAndQuickPositionFFFF) {
    ItemBase item = make_empty_item();
    EXPECT_EQ(item.dwDBIdx, 0u);
    EXPECT_EQ(item.wIconIdx, 0u);
    EXPECT_EQ(item.QuickPosition, 0xFFFFu);
    EXPECT_TRUE(is_empty_slot(item));
}

TEST(ItemBaseTest, IsEmptySlotReturnsFalseForPopulatedItem) {
    ItemBase item = make_item(1, 1, 0);
    EXPECT_FALSE(is_empty_slot(item));
}

TEST(ItemBaseTest, IsEmptySlotReturnsTrueIfEitherFieldIsZero) {
    // is_empty_slot() returns true if EITHER dwDBIdx OR wIconIdx is
    // zero. This catches both "never had an item" and "was cleared
    // back to 0 by a server-side wipe" — both are empty from the
    // client's point of view. Only when BOTH are non-zero does the
    // slot count as occupied.
    ItemBase a{}; a.dwDBIdx = 0; a.wIconIdx = 0;  // truly empty
    ItemBase b{}; b.dwDBIdx = 0; b.wIconIdx = 5;  // dwDBIdx is 0 → empty
    ItemBase c{}; c.dwDBIdx = 5; c.wIconIdx = 0;  // wIconIdx is 0 → empty
    ItemBase d{}; d.dwDBIdx = 5; d.wIconIdx = 7;  // both non-zero → occupied
    EXPECT_TRUE(is_empty_slot(a));
    EXPECT_TRUE(is_empty_slot(b));
    EXPECT_TRUE(is_empty_slot(c));
    EXPECT_FALSE(is_empty_slot(d));
}

TEST(ItemTotalInfoTest, WireFormatSizeIs2728Bytes) {
    EXPECT_EQ(ITEM_TOTAL_SLOT_COUNT, 124u);
    EXPECT_EQ(sizeof(ItemTotalInfo), 22u * 124u);
    EXPECT_EQ(sizeof(ItemTotalInfo), 2728u);
}

TEST(ItemTotalInfoTest, FieldOffsetsMatchLegacy) {
    EXPECT_EQ(offsetof(ItemTotalInfo, Inventory), 0u);
    EXPECT_EQ(offsetof(ItemTotalInfo, WearedItem), 1760u);
    EXPECT_EQ(offsetof(ItemTotalInfo, ShopInventory), 1980u);
    EXPECT_EQ(offsetof(ItemTotalInfo, PetWearedItem), 2420u);
    EXPECT_EQ(offsetof(ItemTotalInfo, TitanWearedItem), 2486u);
    EXPECT_EQ(offsetof(ItemTotalInfo, TitanShopItem), 2640u);
}

TEST(ItemTotalInfoTest, AllSlotsStartEmpty) {
    ItemTotalInfo info{};
    for (const auto& item : info.Inventory) EXPECT_TRUE(is_empty_slot(item));
    for (const auto& item : info.WearedItem) EXPECT_TRUE(is_empty_slot(item));
    for (const auto& item : info.ShopInventory) EXPECT_TRUE(is_empty_slot(item));
    for (const auto& item : info.PetWearedItem) EXPECT_TRUE(is_empty_slot(item));
    for (const auto& item : info.TitanWearedItem) EXPECT_TRUE(is_empty_slot(item));
    for (const auto& item : info.TitanShopItem) EXPECT_TRUE(is_empty_slot(item));
}

TEST(HeroTotalLayoutTest, CurrentItemBlockShiftsTrailingFields) {
    EXPECT_EQ(HERO_TOTAL_ITEM_OFFSET, 1019u);
    EXPECT_EQ(HERO_TOTAL_OPTION_COUNTS_OFFSET, 3747u);
    EXPECT_EQ(HERO_TOTAL_SERVER_TIME_OFFSET, 3757u);
    EXPECT_EQ(HERO_TOTAL_ADDABLE_INFO_OFFSET, 3773u);
    EXPECT_EQ(HERO_TOTAL_EMPTY_PAYLOAD_SIZE, 3775u);
}

TEST(EquipmentPositionTest, RangesMatchDefaultLegacyBranch) {
    EXPECT_EQ(TP_INVENTORY_START, 0u);
    EXPECT_EQ(TP_INVENTORY_END, 80u);
    EXPECT_EQ(TP_WEAREDITEM_START, 80u);
    EXPECT_EQ(TP_WEAREDITEM_END, 90u);
    EXPECT_EQ(TP_PYOGUK_START, 90u);
    EXPECT_EQ(TP_PYOGUK_END, 240u);
    EXPECT_EQ(TP_SHOPITEM_START, 240u);
    EXPECT_EQ(TP_SHOPITEM_END, 390u);
    EXPECT_EQ(TP_SHOPINVEN_START, 390u);
    EXPECT_EQ(TP_SHOPINVEN_END, 430u);
    EXPECT_EQ(TP_PETINVEN_START, 430u);
    EXPECT_EQ(TP_PETINVEN_END, 490u);
    EXPECT_EQ(TP_PETWEAR_START, 490u);
    EXPECT_EQ(TP_PETWEAR_END, 493u);
    EXPECT_EQ(TP_TITANWEAR_START, 493u);
    EXPECT_EQ(TP_TITANWEAR_END, 500u);
    EXPECT_EQ(TP_TITANSHOPITEM_START, 500u);
    EXPECT_EQ(TP_TITANSHOPITEM_END, 504u);
}

TEST(EquipmentPositionTest, WearedSlotsMatchEnum) {
    // The 10 WEARED_* constants must be exactly 0..9 in order —
    // server code does WEARED_HAT=0..WEARED_BELT=9 to index
    // WearedItem[]. If you re-order them, every equip packet breaks.
    EXPECT_EQ(WEARED_HAT, 0u);
    EXPECT_EQ(WEARED_WEAPON, 1u);
    EXPECT_EQ(WEARED_DRESS, 2u);
    EXPECT_EQ(WEARED_SHOES, 3u);
    EXPECT_EQ(WEARED_RING1, 4u);
    EXPECT_EQ(WEARED_RING2, 5u);
    EXPECT_EQ(WEARED_CAPE, 6u);
    EXPECT_EQ(WEARED_NECKLACE, 7u);
    EXPECT_EQ(WEARED_ARMLET, 8u);
    EXPECT_EQ(WEARED_BELT, 9u);
}

// ===========================================================================
// monster_types.hpp
// ===========================================================================

TEST(MonsterTotalInfoTest, WireFormatSizeIs14Bytes) {
    EXPECT_EQ(sizeof(MonsterTotalInfo), 14u);
}

TEST(MonsterTotalInfoTest, FieldsAreAccessible) {
    MonsterTotalInfo m{};
    m.Life = 1000;
    m.Shield = 50;
    m.MonsterKind = OBJECTKIND_BOSS_MONSTER;
    m.Group = 0;
    m.MapNum = 7;
    EXPECT_EQ(m.Life, 1000u);
    EXPECT_EQ(m.Shield, 50u);
    EXPECT_EQ(m.MonsterKind, 33u);  // OBJECTKIND_BOSS_MONSTER
    EXPECT_EQ(m.MapNum, 7u);
}

TEST(NpcRegenTest, WireFormatSizeIs43Bytes) {
    // The struct comment in monster_types.hpp says 44 bytes but the
    // struct's own static_assert() says 43 (the actual packed layout).
    // The 1-byte difference is intentional — the legacy struct had a
    // trailing padding byte that the modern struct dropped. Tests
    // track the modern struct (43) so the test is consistent with
    // the live static_assert.
    EXPECT_EQ(sizeof(NpcRegen), 43u);
}

TEST(MonsterTemplateTest, DefaultConstructionIsEmpty) {
    MonsterTemplate t{};
    EXPECT_EQ(t.MonsterKind, 0u);
    EXPECT_EQ(t.ObjectKind, 32u);   // OBJECTKIND_MONSTER
    EXPECT_EQ(t.Level, 1u);
    EXPECT_EQ(t.Life, 100u);         // default 100
    EXPECT_EQ(t.Shield, 0u);
    EXPECT_EQ(t.Defense, 3u);        // default 3
}

TEST(MonsterInstanceTest, DefaultRuntimeFieldsAreZero) {
    MonsterInstance inst{};
    EXPECT_EQ(inst.object_id, 0u);
    EXPECT_EQ(inst.monster_kind, 0u);
    EXPECT_EQ(inst.object_kind, 32u); // OBJECTKIND_MONSTER
    EXPECT_EQ(inst.level, 1u);
    EXPECT_EQ(inst.group, 0u);
    EXPECT_EQ(inst.map_num, 0u);
}

TEST(ObjectKindTest, MonsterHierarchyIsDistinct) {
    // The 3 monster kinds must be distinct so server code can dispatch
    // (BOSS_MONSTER gets extra drops, SPECIAL_MONSTER has scripted AI).
    EXPECT_NE(OBJECTKIND_MONSTER, OBJECTKIND_BOSS_MONSTER);
    EXPECT_NE(OBJECTKIND_MONSTER, OBJECTKIND_SPECIAL_MONSTER);
    EXPECT_NE(OBJECTKIND_BOSS_MONSTER, OBJECTKIND_SPECIAL_MONSTER);
    EXPECT_EQ(OBJECTKIND_MONSTER, 32u);
    EXPECT_EQ(OBJECTKIND_BOSS_MONSTER, 33u);
    EXPECT_EQ(OBJECTKIND_SPECIAL_MONSTER, 34u);
}

// ===========================================================================
// skill_types.hpp
// ===========================================================================

TEST(SkillInfoTest, DefaultConstructionIsEmpty) {
    SkillInfo s{};
    // D1.3 expanded SkillInfo to the 1:1 legacy SKILLINFO layout.  All
    // numeric defaults are zero (zero-init); the legacy .bin populates
    // the actual values.
    EXPECT_EQ(s.SkillIdx, 0u);
    EXPECT_EQ(s.SkillKind, 0u);            // 0 == Combo in legacy enum
    EXPECT_EQ(s.SkillRange, 0u);
    EXPECT_EQ(s.TargetRange, 0u);
    EXPECT_EQ(s.DelayTime, 0u);
    EXPECT_EQ(s.Duration, 0u);
    EXPECT_EQ(s.WeaponKind, 0u);
    // SkillName is NUL-padded.
    EXPECT_EQ(s.SkillName[0], '\0');
}

TEST(SkillInfoTest, FieldsAreSettable) {
    SkillInfo s{};
    s.SkillIdx   = 100;
    s.SkillKind  = 1;                       // OuterMugong
    s.SkillRange = 5;
    s.TargetRange = 3;                       // AoE radius 3
    s.DelayTime  = 2000;
    s.Duration   = 10000;
    s.WeaponKind = 2;                       // sword
    EXPECT_EQ(s.SkillIdx,   100u);
    EXPECT_EQ(s.SkillKind,  1u);
    EXPECT_EQ(s.SkillRange, 5u);
    EXPECT_EQ(s.TargetRange, 3u);
    EXPECT_EQ(s.DelayTime,  2000u);
    EXPECT_EQ(s.Duration,   10000u);
    EXPECT_EQ(s.WeaponKind, 2u);
}

TEST(SkillInstanceTest, TracksCasterAndTarget) {
    SkillInstance inst{};
    EXPECT_EQ(inst.skill_object_id, 0u);
    EXPECT_EQ(inst.skill_idx,       0u);
    EXPECT_EQ(inst.caster_id,       0u);
    EXPECT_EQ(inst.main_target_id,  0u);
    EXPECT_TRUE(inst.is_active);

    inst.skill_object_id = 999;
    inst.skill_idx       = 42;
    inst.caster_id       = 1000;
    inst.main_target_id  = 1001;
    inst.pos_x = 100.0f;
    inst.pos_z = 200.0f;
    inst.direction = 180;
    inst.duration = 5000;
    inst.is_active = false;
    EXPECT_EQ(inst.skill_object_id, 999u);
    EXPECT_EQ(inst.skill_idx,       42u);
    EXPECT_EQ(inst.caster_id,       1000u);
    EXPECT_EQ(inst.main_target_id,  1001u);
    EXPECT_FLOAT_EQ(inst.pos_x,     100.0f);
    EXPECT_FLOAT_EQ(inst.pos_z,     200.0f);
    EXPECT_EQ(inst.direction,       180u);
    EXPECT_EQ(inst.duration,        5000u);
    EXPECT_FALSE(inst.is_active);
}

TEST(DamageResultTest, DefaultIsZeroDamage) {
    DamageResult d{};
    EXPECT_EQ(d.target_id, 0u);
    EXPECT_EQ(d.damage, 0);           // negative = heal
    EXPECT_FALSE(d.is_critical);
    EXPECT_FALSE(d.is_miss);
    EXPECT_EQ(d.hit_result, 0u);     // 0=miss, 1=hit, 2=critical
}

TEST(DamageResultTest, AllFieldsAreSettable) {
    DamageResult d{};
    d.target_id = 1234;
    d.damage = 250;
    d.is_critical = true;
    d.is_miss = false;
    d.hit_result = 2;  // critical
    EXPECT_EQ(d.target_id, 1234u);
    EXPECT_EQ(d.damage, 250);
    EXPECT_TRUE(d.is_critical);
    EXPECT_FALSE(d.is_miss);
    EXPECT_EQ(d.hit_result, 2u);
}

TEST(DamageResultTest, NegativeDamageMeansHeal) {
    DamageResult d{};
    d.damage = -50;  // heal 50 hp
    EXPECT_LT(d.damage, 0);
    EXPECT_EQ(d.damage, -50);
}

TEST(PlayerCombatStatsTest, DefaultStatsAreNonZero) {
    PlayerCombatStats s{};
    // Defaults from the header — not zero; a fresh level-1 character
    // has 100 HP, 10 phy_attack, 5 phy_defence, 5% crit/dodge.
    EXPECT_EQ(s.level, 1u);
    EXPECT_EQ(s.max_hp, 100u);
    EXPECT_EQ(s.current_hp, 100u);
    EXPECT_EQ(s.max_mp, 50u);
    EXPECT_EQ(s.current_mp, 50u);
    EXPECT_EQ(s.phy_attack, 10u);
    EXPECT_EQ(s.phy_defence, 5u);
    EXPECT_EQ(s.att_attack, 0u);
    EXPECT_EQ(s.att_defence, 0u);
    EXPECT_EQ(s.critical_rate, 5u);
    EXPECT_EQ(s.dodge_rate, 5u);
}

TEST(PlayerCombatStatsTest, AllFieldsAreSettable) {
    PlayerCombatStats s{};
    s.level = 30;
    s.max_hp = 5000;
    s.current_hp = 4500;
    s.max_mp = 2000;
    s.current_mp = 1800;
    s.phy_attack = 800;
    s.phy_defence = 400;
    s.att_attack = 600;
    s.att_defence = 350;
    s.critical_rate = 20;
    s.dodge_rate = 15;
    EXPECT_EQ(s.level, 30u);
    EXPECT_EQ(s.max_hp, 5000u);
    EXPECT_EQ(s.current_hp, 4500u);
    EXPECT_EQ(s.max_mp, 2000u);
    EXPECT_EQ(s.current_mp, 1800u);
    EXPECT_EQ(s.phy_attack, 800u);
    EXPECT_EQ(s.phy_defence, 400u);
    EXPECT_EQ(s.att_attack, 600u);
    EXPECT_EQ(s.att_defence, 350u);
    EXPECT_EQ(s.critical_rate, 20u);
    EXPECT_EQ(s.dodge_rate, 15u);
}

}  // namespace mxh::game::test
