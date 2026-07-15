// item_effects_test.cpp - Tests for mxh::game::resolve_item_effect.
//
// Covers modern/include/mxh/game/item_effects.hpp + src/item_effects.cpp.
// The legacy ItemList.bin parser is not yet implemented in the modern
// server, so resolve_item_effect() uses a small hardcoded table with
// linear scaling per consumable range. These tests pin the exact
// scaling so a future ItemList.bin replacement lands as a deliberate
// contract change, not a silent regression.
//
// Consumable ranges (per item_effects.hpp):
//   1-99   : HP potion       (50, 100, 150, ..., 4950)
//   100-199: MP potion       (30, 60, 90, ..., 2970)
//   200-299: HP+MP potion    (half of HP-only and MP-only at same level)
//   300-399: Buff potion     (buff id 1..100, no HP/MP delta)
//   0, 400+: not consumable  (zero effect)

#include "mxh/game/item_effects.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace mxh::game::test {

// ===========================================================================
// classify_item
// ===========================================================================

TEST(ClassifyItemTest, EmptySlotIsNone) {
    EXPECT_EQ(classify_item(0), ItemEffectKind::None);
}

TEST(ClassifyItemTest, HpPotionRange) {
    EXPECT_EQ(classify_item(1),   ItemEffectKind::HpPotion);
    EXPECT_EQ(classify_item(50),  ItemEffectKind::HpPotion);
    EXPECT_EQ(classify_item(99),  ItemEffectKind::HpPotion);
}

TEST(ClassifyItemTest, MpPotionRange) {
    EXPECT_EQ(classify_item(100), ItemEffectKind::MpPotion);
    EXPECT_EQ(classify_item(150), ItemEffectKind::MpPotion);
    EXPECT_EQ(classify_item(199), ItemEffectKind::MpPotion);
}

TEST(ClassifyItemTest, BothPotionRange) {
    EXPECT_EQ(classify_item(200), ItemEffectKind::BothPotion);
    EXPECT_EQ(classify_item(250), ItemEffectKind::BothPotion);
    EXPECT_EQ(classify_item(299), ItemEffectKind::BothPotion);
}

TEST(ClassifyItemTest, BuffPotionRange) {
    EXPECT_EQ(classify_item(300), ItemEffectKind::BuffPotion);
    EXPECT_EQ(classify_item(350), ItemEffectKind::BuffPotion);
    EXPECT_EQ(classify_item(399), ItemEffectKind::BuffPotion);
}

TEST(ClassifyItemTest, EquipmentIsNotConsumable) {
    // Equipment starts at wIconIdx 1000+ in legacy item tables.
    // Out of consumable range → None.
    EXPECT_EQ(classify_item(400),  ItemEffectKind::None);
    EXPECT_EQ(classify_item(999),  ItemEffectKind::None);
    EXPECT_EQ(classify_item(1000), ItemEffectKind::None);
    EXPECT_EQ(classify_item(55000), ItemEffectKind::None);
    EXPECT_EQ(classify_item(65535), ItemEffectKind::None);
}

// ===========================================================================
// resolve_item_effect — non-consumable
// ===========================================================================

TEST(ResolveItemEffectTest, NonConsumableReturnsZeroEffect) {
    ItemEffect e = resolve_item_effect(0);
    EXPECT_EQ(e.hp_delta, 0);
    EXPECT_EQ(e.mp_delta, 0);
    EXPECT_EQ(e.buff, 0u);

    e = resolve_item_effect(400);
    EXPECT_EQ(e.hp_delta, 0);
    EXPECT_EQ(e.mp_delta, 0);
    EXPECT_EQ(e.buff, 0u);

    e = resolve_item_effect(12345);
    EXPECT_EQ(e.hp_delta, 0);
    EXPECT_EQ(e.mp_delta, 0);
    EXPECT_EQ(e.buff, 0u);
}

// ===========================================================================
// resolve_item_effect — HP potion
// ===========================================================================

TEST(ResolveItemEffectTest, HpPotionScalesLinearly) {
    // Range 1..99 → 50..4950, step 50.
    EXPECT_EQ(resolve_item_effect(1).hp_delta, 50);
    EXPECT_EQ(resolve_item_effect(2).hp_delta, 100);
    EXPECT_EQ(resolve_item_effect(10).hp_delta, 500);
    EXPECT_EQ(resolve_item_effect(50).hp_delta, 2500);
    EXPECT_EQ(resolve_item_effect(99).hp_delta, 4950);
}

TEST(ResolveItemEffectTest, HpPotionNoMpNoBuff) {
    ItemEffect e = resolve_item_effect(50);
    EXPECT_EQ(e.hp_delta, 2500);
    EXPECT_EQ(e.mp_delta, 0);
    EXPECT_EQ(e.buff, 0u);
}

// ===========================================================================
// resolve_item_effect — MP potion
// ===========================================================================

TEST(ResolveItemEffectTest, MpPotionScalesLinearly) {
    // Range 100..199 → 30..3000, step 30.
    // Formula: mp_delta = (w_icon_idx - 99) * 30.
    EXPECT_EQ(resolve_item_effect(100).mp_delta, 30);
    EXPECT_EQ(resolve_item_effect(101).mp_delta, 60);
    EXPECT_EQ(resolve_item_effect(110).mp_delta, 330);
    EXPECT_EQ(resolve_item_effect(150).mp_delta, 1530);
    EXPECT_EQ(resolve_item_effect(199).mp_delta, 3000);
}

TEST(ResolveItemEffectTest, MpPotionNoHpNoBuff) {
    ItemEffect e = resolve_item_effect(150);
    EXPECT_EQ(e.hp_delta, 0);
    EXPECT_EQ(e.mp_delta, 1530);
    EXPECT_EQ(e.buff, 0u);
}

// ===========================================================================
// resolve_item_effect — BothPotion
// ===========================================================================

TEST(ResolveItemEffectTest, BothPotionHalfOfEachAtSameLevel) {
    // Level index = wIconIdx - 200 (0..99).
    // HP delta = (level + 1) * 50 / 2.
    // MP delta = (level + 1) * 30 / 2.
    ItemEffect e = resolve_item_effect(200);
    // level 0: hp = 1*50/2 = 25, mp = 1*30/2 = 15.
    EXPECT_EQ(e.hp_delta, 25);
    EXPECT_EQ(e.mp_delta, 15);

    e = resolve_item_effect(201);
    // level 1: hp = 2*50/2 = 50, mp = 2*30/2 = 30.
    EXPECT_EQ(e.hp_delta, 50);
    EXPECT_EQ(e.mp_delta, 30);

    e = resolve_item_effect(250);
    // level 50: hp = 51*50/2 = 1275, mp = 51*30/2 = 765.
    EXPECT_EQ(e.hp_delta, 1275);
    EXPECT_EQ(e.mp_delta, 765);

    e = resolve_item_effect(299);
    // level 99: hp = 100*50/2 = 2500, mp = 100*30/2 = 1500.
    EXPECT_EQ(e.hp_delta, 2500);
    EXPECT_EQ(e.mp_delta, 1500);
}

// ===========================================================================
// resolve_item_effect — BuffPotion
// ===========================================================================

TEST(ResolveItemEffectTest, BuffPotionSetsBuffId) {
    // Range 300..399 → buff id 1..100.
    EXPECT_EQ(resolve_item_effect(300).buff, 1u);
    EXPECT_EQ(resolve_item_effect(301).buff, 2u);
    EXPECT_EQ(resolve_item_effect(350).buff, 51u);
    EXPECT_EQ(resolve_item_effect(399).buff, 100u);
}

TEST(ResolveItemEffectTest, BuffPotionNoImmediateHpMp) {
    // Buff items apply a timed status, not immediate HP/MP. The
    // legacy buff system is deferred, so HP/MP delta is 0 for now.
    ItemEffect e = resolve_item_effect(350);
    EXPECT_EQ(e.hp_delta, 0);
    EXPECT_EQ(e.mp_delta, 0);
    EXPECT_EQ(e.buff, 51u);
}

// ===========================================================================
// Range boundary stability
// ===========================================================================

TEST(ResolveItemEffectTest, BoundaryTransitions) {
    // 99 → HpPotion, 100 → MpPotion, 199 → MpPotion, 200 → BothPotion,
    // 299 → BothPotion, 300 → BuffPotion, 399 → BuffPotion, 400 → None.
    EXPECT_EQ(resolve_item_effect(99).hp_delta,  4950);
    EXPECT_EQ(resolve_item_effect(99).mp_delta,  0);
    EXPECT_EQ(resolve_item_effect(99).buff,     0u);

    EXPECT_EQ(resolve_item_effect(100).hp_delta, 0);
    EXPECT_EQ(resolve_item_effect(100).mp_delta, 30);

    EXPECT_EQ(resolve_item_effect(199).mp_delta, 3000);

    EXPECT_EQ(resolve_item_effect(200).hp_delta, 25);
    EXPECT_EQ(resolve_item_effect(200).mp_delta, 15);

    EXPECT_EQ(resolve_item_effect(300).buff, 1u);

    // 400 = first non-consumable.
    ItemEffect e = resolve_item_effect(400);
    EXPECT_EQ(e.hp_delta, 0);
    EXPECT_EQ(e.mp_delta, 0);
    EXPECT_EQ(e.buff, 0u);
}

}  // namespace mxh::game::test
