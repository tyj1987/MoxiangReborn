// D4.54 ItemMixAddItem (MP_ITEM_MIX_ADDITEM_SYN) data-plane +
// side-effect dispatcher tests.

#include <mxh/server/item_mix_add_item_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

ItemMixAddItemValidationInput all_ok() {
    ItemMixAddItemValidationInput in{};
    in.table_idx_position = LEGACY_EITEMTABLE_INVENTORY;
    in.item_of_passed = true;
    in.slot_is_locked = false;
    in.is_option_item = false;
    in.has_mix_info = true;
    in.item_kind = LEGACY_EKIND_YOUNGYAK;
    in.durability = 1;
    return in;
}

TEST(ItemMixAddItemOutcome, AllGatesPassIsSuccess) {
    auto in = all_ok();
    EXPECT_EQ(classify_item_mix_add_item_outcome(in),
              ItemMixAddItemOutcome::Success);
}

TEST(ItemMixAddItemOutcome, NotInInventoryFailsFirst) {
    auto in = all_ok();
    in.table_idx_position = 99;
    in.item_of_passed = false;
    EXPECT_EQ(classify_item_mix_add_item_outcome(in),
              ItemMixAddItemOutcome::NotInInventory);
}

TEST(ItemMixAddItemOutcome, ItemMismatch) {
    auto in = all_ok();
    in.item_of_passed = false;
    EXPECT_EQ(classify_item_mix_add_item_outcome(in),
              ItemMixAddItemOutcome::ItemMismatch);
}

TEST(ItemMixAddItemOutcome, SlotLocked) {
    auto in = all_ok();
    in.slot_is_locked = true;
    EXPECT_EQ(classify_item_mix_add_item_outcome(in),
              ItemMixAddItemOutcome::SlotLocked);
}

TEST(ItemMixAddItemOutcome, OptionItem) {
    auto in = all_ok();
    in.is_option_item = true;
    EXPECT_EQ(classify_item_mix_add_item_outcome(in),
              ItemMixAddItemOutcome::OptionItem);
}

TEST(ItemMixAddItemOutcome, NoMixInfo) {
    auto in = all_ok();
    in.has_mix_info = false;
    EXPECT_EQ(classify_item_mix_add_item_outcome(in),
              ItemMixAddItemOutcome::NoMixInfo);
}

TEST(ItemMixAddItemOutcome, NotMixableWhenNonYoungYakHighDurability) {
    auto in = all_ok();
    in.item_kind = 1;  // not youngyak
    in.durability = 5;
    EXPECT_EQ(classify_item_mix_add_item_outcome(in),
              ItemMixAddItemOutcome::NotMixable);
}

TEST(ItemMixAddItemOutcome, YoungYakHighDurabilityIsOk) {
    auto in = all_ok();
    in.item_kind = LEGACY_EKIND_YOUNGYAK;
    in.durability = 5;
    EXPECT_EQ(classify_item_mix_add_item_outcome(in),
              ItemMixAddItemOutcome::Success);
}

TEST(ItemMixAddItemOutcome, JewelHighDurabilityIsOk) {
    auto in = all_ok();
    in.item_kind = LEGACY_EKIND_JEWEL;
    in.durability = 5;
    EXPECT_EQ(classify_item_mix_add_item_outcome(in),
              ItemMixAddItemOutcome::Success);
}

TEST(ItemMixAddItemPlan, SuccessEmitsSetLockThenAck) {
    auto in = all_ok();
    auto plan = item_mix_add_item_side_effect_plan(in, /*pos=*/10);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemMixAddItemSideEffectKind::SetSlotLock);
    EXPECT_EQ(plan.effects[0].item_pos, 10u);
    EXPECT_EQ(plan.effects[1].kind,
              ItemMixAddItemSideEffectKind::BroadcastMixAddItemAck);
    EXPECT_EQ(plan.effects[1].item_pos, 10u);
}

TEST(ItemMixAddItemPlan, NotInInventoryEmitsNack1) {
    auto in = all_ok();
    in.table_idx_position = 99;
    auto plan = item_mix_add_item_side_effect_plan(in, /*pos=*/10);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.error_code, 1u);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemMixAddItemSideEffectKind::BroadcastMixAddItemNack);
    EXPECT_EQ(plan.effects[0].error_code, 1u);
}

TEST(ItemMixAddItemPlan, NotMixableEmitsNack6) {
    auto in = all_ok();
    in.item_kind = 1;
    in.durability = 5;
    auto plan = item_mix_add_item_side_effect_plan(in, /*pos=*/10);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.error_code, 6u);
}

TEST(ItemMixAddItemPlan, PlanIsIdempotent) {
    auto in = all_ok();
    auto a = item_mix_add_item_side_effect_plan(in, 10);
    auto b = item_mix_add_item_side_effect_plan(in, 10);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.send_nack, b.send_nack);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].item_pos, b.effects[i].item_pos);
    }
}
