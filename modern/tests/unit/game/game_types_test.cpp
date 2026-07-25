// mxh/tests/unit/game/game_types_test.cpp
// Phase D6.1 — numeric baseline lock for the gameplay types.
//
// Every constant, struct size, enum value, and helper result here is
// matched against the legacy CommonStruct.h / CommonGameDefine.h /
// etc.  Any future change to a baseline must be a deliberate cross-
// platform port, not a silent drift.  The unit tests are the
// "no-drift" safety net for the rest of Phase D (D1 SkillManager,
// D2 BattleFactory, D3 QuestManager, D4 商城/物品/仓库/邮件/帮派/队伍,
// D5 MurimNet) — when those sub-phases port more data, they extend
// this file with their own numbers, but they do NOT loosen any of
// the baselines locked here.

#include "mxh/game/item_effects.hpp"
#include "mxh/game/item_types.hpp"
#include "mxh/game/monster_types.hpp"
#include "mxh/game/skill_types.hpp"

#include <gtest/gtest.h>

#include <cstdint>

using namespace mxh::game;

// -------------------------------------------------------------------------
// ObjectKind constants (from legacy CommonGameDefine.h eObjectKind).
// -------------------------------------------------------------------------

TEST(GameObjectKind, MonsterKindIs32) {
    EXPECT_EQ(OBJECTKIND_MONSTER, 32);
}
TEST(GameObjectKind, BossMonsterIs33) {
    EXPECT_EQ(OBJECTKIND_BOSS_MONSTER, 33);
}
TEST(GameObjectKind, SpecialMonsterIs34) {
    EXPECT_EQ(OBJECTKIND_SPECIAL_MONSTER, 34);
}
TEST(GameObjectKind, FieldBossIs35) {
    EXPECT_EQ(OBJECTKIND_FIELD_BOSS, 35);
}
TEST(GameObjectKind, FieldSubIs36) {
    EXPECT_EQ(OBJECTKIND_FIELD_SUB, 36);
}
TEST(GameObjectKind, TogetherPlayerIs37) {
    EXPECT_EQ(OBJECTKIND_TOGETHER_PLAY, 37);
}
TEST(GameObjectKind, TitanIs41) {
    EXPECT_EQ(OBJECTKIND_TITAN, 41);
}

// -------------------------------------------------------------------------
// Monster group / regen caps (from legacy CommonGameDefine.h).
// -------------------------------------------------------------------------

TEST(GameMonsterCaps, MaxGroupNumIs200) {
    EXPECT_EQ(MAX_MONSTER_GROUPNUM, 200);
}
TEST(GameMonsterCaps, MaxRegenNumIs100) {
    EXPECT_EQ(MAX_MONSTER_REGEN_NUM, 100);
}
TEST(GameMonsterCaps, RegenRandomRangeIs1500) {
    EXPECT_FLOAT_EQ(MONSTER_REGEN_RANDOM_RANGE, 1500.0f);
}

// -------------------------------------------------------------------------
// MonsterAIState enum (legacy AI state machine).
// -------------------------------------------------------------------------

TEST(GameMonsterAI, EnumValuesMatchLegacy) {
    EXPECT_EQ(static_cast<std::uint8_t>(MonsterAIState::Idle),   0u);
    EXPECT_EQ(static_cast<std::uint8_t>(MonsterAIState::Patrol), 1u);
    EXPECT_EQ(static_cast<std::uint8_t>(MonsterAIState::Chase),  2u);
    EXPECT_EQ(static_cast<std::uint8_t>(MonsterAIState::Attack), 3u);
    EXPECT_EQ(static_cast<std::uint8_t>(MonsterAIState::Return), 4u);
    EXPECT_EQ(static_cast<std::uint8_t>(MonsterAIState::Dead),   5u);
}

// -------------------------------------------------------------------------
// MONSTER_TOTALINFO (14 bytes packed) — wire format for MonsterAdd (proto=37).
// -------------------------------------------------------------------------

TEST(GameMonsterWire, MonsterTotalInfoIs14Bytes) {
    MonsterTotalInfo m{};
    EXPECT_EQ(sizeof(m), 14u);
    EXPECT_EQ(offsetof(MonsterTotalInfo, Life), 0u);
    EXPECT_EQ(offsetof(MonsterTotalInfo, Shield), 4u);
    EXPECT_EQ(offsetof(MonsterTotalInfo, MonsterKind), 8u);
    EXPECT_EQ(offsetof(MonsterTotalInfo, Group), 10u);
    EXPECT_EQ(offsetof(MonsterTotalInfo, MapNum), 12u);
}

TEST(GameMonsterWire, NpcRegenIs43Bytes) {
    NpcRegen r{};
    EXPECT_EQ(sizeof(r), 43u);
}

// -------------------------------------------------------------------------
// ItemBase / ItemTotalInfo (1:1 with CommonStruct.h).
// -------------------------------------------------------------------------

TEST(GameItemWire, ItemBaseIs22Bytes) {
    ItemBase b{};
    EXPECT_EQ(sizeof(b), 22u);
    EXPECT_EQ(offsetof(ItemBase, dwDBIdx),       0u);
    EXPECT_EQ(offsetof(ItemBase, wIconIdx),      4u);
    EXPECT_EQ(offsetof(ItemBase, Position),      6u);
    EXPECT_EQ(offsetof(ItemBase, Durability),    8u);
    EXPECT_EQ(offsetof(ItemBase, RareIdx),       12u);
    EXPECT_EQ(offsetof(ItemBase, QuickPosition), 16u);
    EXPECT_EQ(offsetof(ItemBase, ItemParam),     18u);
}

TEST(GameItemWire, ItemTotalInfoIs110Slots) {
    // 80 inventory + 10 weared + 20 shop = 110 ItemBase slots.
    ItemTotalInfo items{};
    EXPECT_EQ(SLOT_INVENTORY_NUM,    80);
    EXPECT_EQ(WEARED_ITEM_MAX,      10);
    EXPECT_EQ(TABCELL_SHOPINVEN_NUM, 20);
    EXPECT_EQ(SLOT_PYOGUK_NUM,       20);
    EXPECT_EQ(sizeof(items), 22u * 110u);
    EXPECT_EQ(sizeof(SendItemTotalInfoLocal), 4u + 22u * 110u);
}

TEST(GameItemWire, WearedSlotConstants) {
    EXPECT_EQ(WEARED_HAT,       0);
    EXPECT_EQ(WEARED_WEAPON,    1);
    EXPECT_EQ(WEARED_DRESS,     2);
    EXPECT_EQ(WEARED_SHOES,     3);
    EXPECT_EQ(WEARED_RING1,     4);
    EXPECT_EQ(WEARED_RING2,     5);
    EXPECT_EQ(WEARED_CAPE,      6);
    EXPECT_EQ(WEARED_NECKLACE,  7);
    EXPECT_EQ(WEARED_ARMLET,    8);
    EXPECT_EQ(WEARED_BELT,      9);
}

TEST(GameItemWire, PositionRanges) {
    EXPECT_EQ(TP_INVENTORY_START, 0);
    EXPECT_EQ(TP_INVENTORY_END,   80);
    EXPECT_EQ(TP_WEAREDITEM_START, 80);
    EXPECT_EQ(TP_WEAREDITEM_END,   90);
    EXPECT_EQ(TP_SHOPINVEN_START,  90);
    EXPECT_EQ(TP_SHOPINVEN_END,    110);
    EXPECT_EQ(TP_PYOGUK_START,     110);
    EXPECT_EQ(TP_PYOGUK_END,       130);
}

TEST(GameItemHelpers, EmptyItemSlotSentinel) {
    // Empty slot uses dwDBIdx=0, QuickPosition=0xFFFF.
    auto e = make_empty_item();
    EXPECT_EQ(e.dwDBIdx, 0u);
    EXPECT_EQ(e.wIconIdx, 0u);
    EXPECT_EQ(e.QuickPosition, 0xFFFFu);
    EXPECT_TRUE(is_empty_slot(e));
}

TEST(GameItemHelpers, FilledItemIsNotEmpty) {
    auto item = make_item(/*db_idx=*/100, /*icon_idx=*/5, /*pos=*/10);
    EXPECT_EQ(item.dwDBIdx,  100u);
    EXPECT_EQ(item.wIconIdx, 5u);
    EXPECT_EQ(item.Position, 10u);
    EXPECT_EQ(item.ItemParam, 1u);  // default count
    EXPECT_FALSE(is_empty_slot(item));
}

// -------------------------------------------------------------------------
// ItemEffectKind + classify_item (1:1 with item_effects.hpp).
// -------------------------------------------------------------------------

TEST(GameItemEffects, ClassifyRangeBoundaries) {
    EXPECT_EQ(classify_item(0),    ItemEffectKind::None);
    EXPECT_EQ(classify_item(1),    ItemEffectKind::HpPotion);
    EXPECT_EQ(classify_item(99),   ItemEffectKind::HpPotion);
    EXPECT_EQ(classify_item(100),  ItemEffectKind::MpPotion);
    EXPECT_EQ(classify_item(199),  ItemEffectKind::MpPotion);
    EXPECT_EQ(classify_item(200),  ItemEffectKind::BothPotion);
    EXPECT_EQ(classify_item(299),  ItemEffectKind::BothPotion);
    EXPECT_EQ(classify_item(300),  ItemEffectKind::BuffPotion);
    EXPECT_EQ(classify_item(399),  ItemEffectKind::BuffPotion);
    EXPECT_EQ(classify_item(400),  ItemEffectKind::None);
    EXPECT_EQ(classify_item(65535u), ItemEffectKind::None);
}

TEST(GameItemEffects, ResolveHpPotionLinear) {
    // HP potions: range 1..99 → 50..4950 HP, step 50.
    // Formula: hp_delta = wIconIdx * 50.
    EXPECT_EQ(resolve_item_effect(1).hp_delta,   50);
    EXPECT_EQ(resolve_item_effect(2).hp_delta,   100);
    EXPECT_EQ(resolve_item_effect(50).hp_delta,  2500);
    EXPECT_EQ(resolve_item_effect(99).hp_delta,  4950);
    EXPECT_EQ(resolve_item_effect(50).mp_delta,  0);
    EXPECT_EQ(resolve_item_effect(50).buff,     0u);
}

TEST(GameItemEffects, ResolveMpPotionLinear) {
    // MP potions: range 100..199 → 30..2970 MP, step 30.
    // Formula: mp_delta = (wIconIdx - 99) * 30.
    EXPECT_EQ(resolve_item_effect(100).mp_delta, 30);
    EXPECT_EQ(resolve_item_effect(150).mp_delta, 1530);
    EXPECT_EQ(resolve_item_effect(199).mp_delta, 3000);
    EXPECT_EQ(resolve_item_effect(100).hp_delta, 0);
}

TEST(GameItemEffects, ResolveBothPotionHalfHalf) {
    // Combined potion 200..299: each half of the equivalent single-
    // stat potion at the same level index.
    EXPECT_EQ(resolve_item_effect(200).hp_delta, 25);   // (0+1)*50/2
    EXPECT_EQ(resolve_item_effect(200).mp_delta, 15);   // (0+1)*30/2
    EXPECT_EQ(resolve_item_effect(299).hp_delta, 2500); // (99+1)*50/2
    EXPECT_EQ(resolve_item_effect(299).mp_delta, 1500); // (99+1)*30/2
}

TEST(GameItemEffects, ResolveBuffPotionHasBuff) {
    // Buff potions: buff id = wIconIdx - 300 + 1, no HP/MP delta.
    EXPECT_EQ(resolve_item_effect(300).buff, 1u);
    EXPECT_EQ(resolve_item_effect(350).buff, 51u);
    EXPECT_EQ(resolve_item_effect(399).buff, 100u);
    EXPECT_EQ(resolve_item_effect(300).hp_delta, 0);
    EXPECT_EQ(resolve_item_effect(300).mp_delta, 0);
}

TEST(GameItemEffects, ResolveNonConsumableIsNoOp) {
    auto e = resolve_item_effect(0);
    EXPECT_EQ(e.hp_delta, 0);
    EXPECT_EQ(e.mp_delta, 0);
    EXPECT_EQ(e.buff, 0u);
    auto e2 = resolve_item_effect(400);
    EXPECT_EQ(e2.hp_delta, 0);
    EXPECT_EQ(e2.mp_delta, 0);
    EXPECT_EQ(e2.buff, 0u);
}

// -------------------------------------------------------------------------
// MonsterTemplate — default templates (Phase 10c P0 placeholder values).
// These will be replaced by MonsterList.bin data once that's parsed;
// for now we lock the placeholder numbers so future drift is visible.
// -------------------------------------------------------------------------

TEST(GameMonsterTemplate, DefaultTemplatesHaveThreeEntries) {
    auto t = get_default_templates();
    ASSERT_EQ(t.size(), 3u);
}

TEST(GameMonsterTemplate, Template0DoksaLevel1) {
    auto t = get_default_templates();
    EXPECT_EQ(t[0].MonsterKind, 0);
    EXPECT_EQ(t[0].Level,       1);
    EXPECT_EQ(t[0].Life,        80);
    EXPECT_EQ(t[0].Shield,      0);
    EXPECT_EQ(t[0].ExpPoint,    5);
    EXPECT_EQ(t[0].AttackMin,   3);
    EXPECT_EQ(t[0].AttackMax,   8);
    EXPECT_EQ(t[0].Defense,     2);
    EXPECT_FALSE(t[0].Aggressive);
}

TEST(GameMonsterTemplate, Template1LangduLevel3Aggressive) {
    auto t = get_default_templates();
    EXPECT_EQ(t[1].MonsterKind, 1);
    EXPECT_EQ(t[1].Level,       3);
    EXPECT_EQ(t[1].Life,        150);
    EXPECT_EQ(t[1].Shield,      10);
    EXPECT_EQ(t[1].ExpPoint,    15);
    EXPECT_EQ(t[1].AttackMin,   8);
    EXPECT_EQ(t[1].AttackMax,   20);
    EXPECT_EQ(t[1].Defense,     5);
    EXPECT_TRUE(t[1].Aggressive);
}

TEST(GameMonsterTemplate, Template2HeifengLevel10) {
    auto t = get_default_templates();
    EXPECT_EQ(t[2].MonsterKind, 2);
    EXPECT_EQ(t[2].Level,       10);
    EXPECT_EQ(t[2].Life,        500);
    EXPECT_EQ(t[2].Shield,      50);
    EXPECT_EQ(t[2].ExpPoint,    80);
    EXPECT_EQ(t[2].AttackMin,   25);
    EXPECT_EQ(t[2].AttackMax,   60);
    EXPECT_EQ(t[2].Defense,     15);
    EXPECT_TRUE(t[2].Aggressive);
}

TEST(GameMonsterTemplate, MakeMonsterTotalinfoCopiesState) {
    MonsterInstance m{};
    m.current_life    = 70;
    m.current_shield  = 5;
    m.monster_kind    = 2;
    m.group           = 7;
    m.map_num         = 12;
    auto info = make_monster_totalinfo(m);
    EXPECT_EQ(info.Life,        70u);
    EXPECT_EQ(info.Shield,      5u);
    EXPECT_EQ(info.MonsterKind, 2u);
    EXPECT_EQ(info.Group,       7u);
    EXPECT_EQ(info.MapNum,      12u);
}

TEST(GameMonsterTemplate, DefaultSpawnPointsHasFiveForMap12) {
    auto s = get_default_spawn_points(12);
    ASSERT_EQ(s.size(), 5u);
    // ObjectIDs in the reserved monster range (50000+).
    for (std::size_t i = 0; i < s.size(); ++i) {
        EXPECT_EQ(s[i].dwObjectID, 50000u + i);
        EXPECT_EQ(s[i].MapNum,     12u);
        // Cycle through 3 templates.
        EXPECT_EQ(s[i].NpcKind, static_cast<std::uint16_t>(i % 3));
    }
}

// -------------------------------------------------------------------------
// SkillInfo / PlayerCombatStats defaults.
// -------------------------------------------------------------------------

TEST(GameSkillInfo, DefaultFieldsAreSane) {
    SkillInfo s{};
    EXPECT_EQ(s.skill_idx,   0u);
    EXPECT_EQ(s.skill_kind,  0u);
    EXPECT_EQ(s.skill_range, 1u);
    EXPECT_EQ(s.delay_time,  1000u);
    EXPECT_EQ(s.phy_attack,  10u);
    EXPECT_EQ(s.att_rate,    100u);
    EXPECT_EQ(s.critical_rate, 5u);
}

TEST(GameCombatStats, DefaultFieldsAreSane) {
    PlayerCombatStats c{};
    EXPECT_EQ(c.level,         1u);
    EXPECT_EQ(c.max_hp,        100u);
    EXPECT_EQ(c.current_hp,    100u);
    EXPECT_EQ(c.max_mp,        50u);
    EXPECT_EQ(c.current_mp,    50u);
    EXPECT_EQ(c.phy_attack,    10u);
    EXPECT_EQ(c.phy_defence,   5u);
    EXPECT_EQ(c.critical_rate, 5u);
    EXPECT_EQ(c.dodge_rate,    5u);
}

TEST(GameDamageResult, DefaultFieldsAreZero) {
    DamageResult d{};
    EXPECT_EQ(d.target_id,  0u);
    EXPECT_EQ(d.damage,     0);
    EXPECT_FALSE(d.is_critical);
    EXPECT_FALSE(d.is_miss);
    EXPECT_EQ(d.hit_result, 0u);
}
