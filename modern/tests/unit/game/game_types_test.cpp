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

TEST(GameItemWire, ItemTotalInfoMatchesDefaultLegacyLayout) {
    ItemTotalInfo items{};
    EXPECT_EQ(SLOT_INVENTORY_NUM, 80);
    EXPECT_EQ(WEARED_ITEM_MAX, 10);
    EXPECT_EQ(TABCELL_SHOPINVEN_NUM, 20);
    EXPECT_EQ(SLOT_PYOGUK_NUM, 150);
    EXPECT_EQ(SLOT_PETWEAR_NUM, 3);
    EXPECT_EQ(SLOT_TITANWEAR_NUM, 7);
    EXPECT_EQ(SLOT_TITANSHOPITEM_NUM, 4);
    EXPECT_EQ(sizeof(items), 22u * 124u);
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
    EXPECT_EQ(TP_INVENTORY_END, 80);
    EXPECT_EQ(TP_WEAREDITEM_START, 80);
    EXPECT_EQ(TP_WEAREDITEM_END, 90);
    EXPECT_EQ(TP_PYOGUK_START, 90);
    EXPECT_EQ(TP_PYOGUK_END, 240);
    EXPECT_EQ(TP_SHOPITEM_START, 240);
    EXPECT_EQ(TP_SHOPITEM_END, 390);
    EXPECT_EQ(TP_SHOPINVEN_START, 390);
    EXPECT_EQ(TP_SHOPINVEN_END, 430);
    EXPECT_EQ(TP_PETINVEN_START, 430);
    EXPECT_EQ(TP_PETINVEN_END, 490);
    EXPECT_EQ(TP_PETWEAR_START, 490);
    EXPECT_EQ(TP_PETWEAR_END, 493);
    EXPECT_EQ(TP_TITANWEAR_START, 493);
    EXPECT_EQ(TP_TITANWEAR_END, 500);
    EXPECT_EQ(TP_TITANSHOPITEM_START, 500);
    EXPECT_EQ(TP_TITANSHOPITEM_END, 504);
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
    EXPECT_EQ(t[0].MonsterKind, 1);
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
    EXPECT_EQ(t[1].MonsterKind, 2);
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
    EXPECT_EQ(t[2].MonsterKind, 3);
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
        EXPECT_EQ(s[i].NpcKind, static_cast<std::uint16_t>(i % 3 + 1));
    }
}

// -------------------------------------------------------------------------
// D6.3 — spawn-geometry numerics.  The legacy engine places 5 monsters
// in a 5-fold symmetric ring around the map center, with a 30-tile
// radius and the center at (150, 150).  Lock these values so any
// future change to the spawn pattern is visible.
// -------------------------------------------------------------------------

TEST(GameSpawnGeometry, Map12UsesOriginalLoginPointRadius300) {
    // Center is LoginPoint 2012 from the shipped original resource.
    // default-spawn loop uses; if they change, the AI pathing test
    // (Phase D2) and MapServer respawn (Phase B.5) must both be
    // re-verified.
    auto s = get_default_spawn_points(12);
    ASSERT_EQ(s.size(), 5u);
    // Every spawn must lie on the ring of radius 30 around (150, 150).
    constexpr float cx   = 27189.0f;
    constexpr float cz   = 27361.0f;
    constexpr float r    = 300.0f;
    constexpr float r_sq = r * r;
    for (const auto& sp : s) {
        const float dx = sp.PosX - cx;
        const float dz = sp.PosZ - cz;
        // distance_sq_2d should be exactly r*r.
        EXPECT_NEAR(dx * dx + dz * dz, r_sq, 1.0f);
        // Y always 0 (ground level).
        EXPECT_FLOAT_EQ(sp.PosY, 0.0f);
    }
}

TEST(GameSpawnGeometry, AnglesAreFiveFoldSymmetric) {
    // ang_i = i * 2 * pi / 5  -> 0, 2pi/5, 4pi/5, 6pi/5, 8pi/5.
    auto s = get_default_spawn_points(12);
    ASSERT_EQ(s.size(), 5u);
    constexpr float two_pi = 2.0f * 3.14159f;
    for (std::size_t i = 0; i < s.size(); ++i) {
        const float expected_angle = static_cast<float>(i) * two_pi / 5.0f;
        EXPECT_NEAR(s[i].Angle, expected_angle, 1e-4f);
    }
}

// -------------------------------------------------------------------------
// D6.3 — distance_sq_2d.  Used by the AI to compute aggro range and
// by MapServer for despawn checks.
// -------------------------------------------------------------------------

TEST(GameGeometry, DistanceSq2DIsZeroForSamePoint) {
    EXPECT_FLOAT_EQ(distance_sq_2d(0, 0, 0, 0), 0.0f);
    EXPECT_FLOAT_EQ(distance_sq_2d(150.0f, 150.0f, 150.0f, 150.0f), 0.0f);
}

TEST(GameGeometry, DistanceSq2DIsEuclidean) {
    // 3-4-5 triangle: distance² = 3² + 4² = 25.
    EXPECT_FLOAT_EQ(distance_sq_2d(0.0f, 0.0f, 3.0f, 4.0f), 25.0f);
    EXPECT_FLOAT_EQ(distance_sq_2d(0.0f, 0.0f, -3.0f, -4.0f), 25.0f);
    EXPECT_FLOAT_EQ(distance_sq_2d(0.0f, 0.0f, 5.0f, 12.0f), 169.0f);
}

TEST(GameGeometry, DistanceSq2DIsSymmetric) {
    // d²(p, q) == d²(q, p).
    for (int x = -10; x <= 10; x += 3) {
        for (int z = -10; z <= 10; z += 3) {
            const float a = distance_sq_2d(0.0f, 0.0f,
                                            static_cast<float>(x),
                                            static_cast<float>(z));
            const float b = distance_sq_2d(static_cast<float>(x),
                                            static_cast<float>(z),
                                            0.0f, 0.0f);
            EXPECT_FLOAT_EQ(a, b);
        }
    }
}

TEST(GameGeometry, DistanceSq2DIsTranslationInvariant) {
    // d²(p+t, q+t) == d²(p, q) for any t.
    for (int t = -100; t <= 100; t += 25) {
        const float a = distance_sq_2d(10.0f, 20.0f, 30.0f, 40.0f);
        const float b = distance_sq_2d(10.0f + t, 20.0f + t,
                                       30.0f + t, 40.0f + t);
        EXPECT_FLOAT_EQ(a, b);
    }
}

// -------------------------------------------------------------------------
// SkillInfo / PlayerCombatStats defaults.
// -------------------------------------------------------------------------

TEST(GameSkillInfo, DefaultFieldsAreSane) {
    SkillInfo s{};
    // 1:1 port: SkillIdx is uint16_t; legacy default is 0.
    EXPECT_EQ(s.SkillIdx,   0u);
    // SkillName NUL-padded.
    EXPECT_EQ(s.SkillName[0], '\0');
    // Simplified view of a default-constructed skill.
    auto simple = to_simple(s);
    EXPECT_EQ(simple.skill_kind,  SkillKind::Combo);
    EXPECT_EQ(simple.skill_range, 0u);   // not set, default 0
    EXPECT_EQ(simple.delay_time,  0u);   // not set, default 0
    EXPECT_EQ(simple.phy_attack,  0.0f);
}

// -------------------------------------------------------------------------
// SkillKind enum — 1:1 with the legacy SKILLKIND in
// `墨香【源码】\[CC]Header\CommonStruct.h` lines 2676-2688.  Each
// value is locked so a future port of SkillList.bin can use the enum
// without a numeric drift.
// -------------------------------------------------------------------------

TEST(GameSkillKind, ComboIsZero) {
    EXPECT_EQ(static_cast<std::uint8_t>(SkillKind::Combo), 0u);
}
TEST(GameSkillKind, OuterMugongIs1) {
    EXPECT_EQ(static_cast<std::uint8_t>(SkillKind::OuterMugong), 1u);
}
TEST(GameSkillKind, InnerMugongIs2) {
    EXPECT_EQ(static_cast<std::uint8_t>(SkillKind::InnerMugong), 2u);
}
TEST(GameSkillKind, SimbubIs3) {
    EXPECT_EQ(static_cast<std::uint8_t>(SkillKind::Simbub), 3u);
}
TEST(GameSkillKind, JinbubIs4) {
    EXPECT_EQ(static_cast<std::uint8_t>(SkillKind::Jinbub), 4u);
}
TEST(GameSkillKind, MiningIs5) {
    EXPECT_EQ(static_cast<std::uint8_t>(SkillKind::Mining), 5u);
}
TEST(GameSkillKind, CollectionIs6) {
    EXPECT_EQ(static_cast<std::uint8_t>(SkillKind::Collection), 6u);
}
TEST(GameSkillKind, HuntIs7) {
    EXPECT_EQ(static_cast<std::uint8_t>(SkillKind::Hunt), 7u);
}
TEST(GameSkillKind, TitanIs8) {
    EXPECT_EQ(static_cast<std::uint8_t>(SkillKind::Titan), 8u);
}
TEST(GameSkillKind, MaxIs9) {
    // SKILLKIND_MAX is a count sentinel (==9), not a valid skill kind.
    EXPECT_EQ(static_cast<std::uint8_t>(SkillKind::Max), 9u);
}
TEST(GameSkillKind, EnumIsUint8) {
    // SkillKind is stored as u8 in the legacy wire format; the modern
    // enum must also fit in 1 byte.
    static_assert(sizeof(SkillKind) == 1, "SkillKind must be 1 byte");
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

// -------------------------------------------------------------------------
// D1.1 — get_default_skills() returns 4 placeholder skills that cover
// the most common archetypes (single-target physical, multi-hit
// combo, self-heal, AoE).  These will be replaced by SkillList.bin
// parsing in D1.3; the values are locked here so any future drift is
// visible.
// -------------------------------------------------------------------------

TEST(GameSkillTable, DefaultSkillsHasFourEntries) {
    auto s = get_default_skills();
    ASSERT_EQ(s.size(), 4u);
}

TEST(GameSkillTable, Skill1BasicStrikeOuterMugong) {
    auto s = get_default_skills();
    EXPECT_EQ(s[0].SkillIdx, 1u);
    EXPECT_STREQ(s[0].SkillName, "BasicStrike");
    EXPECT_EQ(s[0].SkillKind, static_cast<std::uint16_t>(SkillKind::OuterMugong));
    EXPECT_EQ(s[0].SkillRange, 1u);
    EXPECT_EQ(s[0].TargetRange, 0u);
    EXPECT_EQ(s[0].DelayTime, 1000u);
    EXPECT_EQ(s[0].UpPhyAttack[0], 15.0f);
    EXPECT_EQ(s[0].NeedNaeRyuk[0], 5u);
}

TEST(GameSkillTable, Skill2TripleCombo) {
    auto s = get_default_skills();
    EXPECT_EQ(s[1].SkillIdx, 2u);
    EXPECT_STREQ(s[1].SkillName, "TripleCombo");
    EXPECT_EQ(s[1].SkillKind, static_cast<std::uint16_t>(SkillKind::Combo));
    EXPECT_EQ(s[1].SkillRange, 2u);
    EXPECT_EQ(s[1].DelayTime, 2500u);
    EXPECT_EQ(s[1].UpPhyAttack[0], 12.0f);
    EXPECT_EQ(s[1].NeedNaeRyuk[0], 15u);
}

TEST(GameSkillTable, Skill3HealSelfSimbub) {
    auto s = get_default_skills();
    EXPECT_EQ(s[2].SkillIdx, 3u);
    EXPECT_STREQ(s[2].SkillName, "HealSelf");
    EXPECT_EQ(s[2].SkillKind, static_cast<std::uint16_t>(SkillKind::Simbub));
    EXPECT_EQ(s[2].SkillRange, 0u);     // self-cast
    EXPECT_EQ(s[2].DelayTime, 5000u);
    EXPECT_EQ(s[2].NeedNaeRyuk[0], 20u);
}

TEST(GameSkillTable, Skill4WhirlwindAoe) {
    auto s = get_default_skills();
    EXPECT_EQ(s[3].SkillIdx, 4u);
    EXPECT_STREQ(s[3].SkillName, "Whirlwind");
    EXPECT_EQ(s[3].SkillKind, static_cast<std::uint16_t>(SkillKind::OuterMugong));
    EXPECT_EQ(s[3].SkillRange, 3u);
    EXPECT_EQ(s[3].TargetRange, 200u);   // 2-tile AoE radius
    EXPECT_EQ(s[3].DelayTime, 4000u);
    EXPECT_EQ(s[3].UpPhyAttack[0], 20.0f);
    EXPECT_EQ(s[3].StunRate[0], 10.0f);
    EXPECT_EQ(s[3].NeedNaeRyuk[0], 30u);
}

TEST(GameSkillTable, SkillIdsAreUnique) {
    auto s = get_default_skills();
    for (std::size_t i = 0; i < s.size(); ++i) {
        for (std::size_t j = i + 1; j < s.size(); ++j) {
            EXPECT_NE(s[i].SkillIdx, s[j].SkillIdx)
                << "duplicate SkillIdx at " << i << " and " << j;
        }
    }
}
