// item_mix_add_item_side_effect_runtime_test.cpp
//
// Verifies apply_item_mix_add_item_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_MIX_ADDITEM_SYN
// side-effect chain) walks the data-plane plan and dispatches each
// entry: lock-then-ACK on success / the 6-way gate NACK code on
// failure.

#include <mxh/server/item_mix_add_item_side_effect.hpp>
#include <mxh/server/item_mix_add_item_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::ItemMixAddItemSideEffectKind;
using mxh::server::ItemMixAddItemSideEffectSink;
using mxh::server::LEGACY_EITEMTABLE_INVENTORY;
using mxh::server::LEGACY_EKIND_JEWEL;
using mxh::server::LEGACY_EKIND_YOUNGYAK;
using mxh::server::LEGACY_MIX_ADDITEM_ERR_ITEM_MISMATCH;
using mxh::server::LEGACY_MIX_ADDITEM_ERR_NOT_IN_INVEN;
using mxh::server::LEGACY_MIX_ADDITEM_ERR_NOT_MIXABLE;
using mxh::server::LEGACY_MIX_ADDITEM_ERR_NO_MIX_INFO;
using mxh::server::LEGACY_MIX_ADDITEM_ERR_OPTION_ITEM;
using mxh::server::LEGACY_MIX_ADDITEM_ERR_SLOT_LOCKED;
using mxh::server::ItemMixAddItemValidationInput;
using mxh::server::apply_item_mix_add_item_side_effects;
using mxh::server::item_mix_add_item_side_effect_plan;

class RecordingSink final : public ItemMixAddItemSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint16_t last_item_pos = 0;
    std::uint16_t last_error_code = 0;
    std::size_t ack_count = 0;
    std::size_t nack_count = 0;
    std::size_t lock_count = 0;

    void set_slot_lock(std::uint16_t item_pos) override {
        calls.push_back("lock");
        last_item_pos = item_pos;
        ++lock_count;
    }
    void broadcast_mix_add_item_ack(std::uint16_t item_pos) override {
        calls.push_back("ack");
        last_item_pos = item_pos;
        ++ack_count;
    }
    void broadcast_mix_add_item_nack(std::uint16_t item_pos,
                                     std::uint16_t error_code) override {
        calls.push_back("nack");
        last_item_pos = item_pos;
        last_error_code = error_code;
        ++nack_count;
    }
};

ItemMixAddItemValidationInput PassingGates() {
    ItemMixAddItemValidationInput in;
    in.table_idx_position = LEGACY_EITEMTABLE_INVENTORY;
    in.item_of_passed = true;
    in.slot_is_locked = false;
    in.is_option_item = false;
    in.has_mix_info = true;
    in.item_kind = LEGACY_EKIND_YOUNGYAK;
    in.durability = 5;
    return in;
}

}  // namespace

TEST(ApplyItemMixAddItemSideEffects, SuccessEmitsLockThenAckInOrder) {
    auto in = PassingGates();
    auto plan = item_mix_add_item_side_effect_plan(in, /*item_pos=*/17);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemMixAddItemSideEffectKind::SetSlotLock);
    EXPECT_EQ(plan.effects[1].kind,
              ItemMixAddItemSideEffectKind::BroadcastMixAddItemAck);
    EXPECT_EQ(plan.effects[0].item_pos, 17u);
    EXPECT_EQ(plan.effects[1].item_pos, 17u);

    RecordingSink sink;
    auto out = apply_item_mix_add_item_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 2u);
    EXPECT_EQ(out.slot_locks, 1u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"lock", "ack"}));
    EXPECT_EQ(sink.last_item_pos, 17u);
    EXPECT_EQ(sink.lock_count, 1u);
    EXPECT_EQ(sink.ack_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyItemMixAddItemSideEffects, NotInInventoryEmitsNackCode1) {
    auto in = PassingGates();
    in.table_idx_position = LEGACY_EITEMTABLE_INVENTORY + 1;
    auto plan = item_mix_add_item_side_effect_plan(in, 3);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.error_code, LEGACY_MIX_ADDITEM_ERR_NOT_IN_INVEN);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemMixAddItemSideEffectKind::BroadcastMixAddItemNack);
    EXPECT_EQ(plan.effects[0].error_code,
              LEGACY_MIX_ADDITEM_ERR_NOT_IN_INVEN);

    RecordingSink sink;
    auto out = apply_item_mix_add_item_side_effects(plan, sink);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"nack"}));
    EXPECT_EQ(sink.last_error_code, LEGACY_MIX_ADDITEM_ERR_NOT_IN_INVEN);
    EXPECT_EQ(sink.last_item_pos, 3u);
}

TEST(ApplyItemMixAddItemSideEffects, ItemMismatchEmitsNackCode2) {
    auto in = PassingGates();
    in.item_of_passed = false;
    auto plan = item_mix_add_item_side_effect_plan(in, 3);
    EXPECT_EQ(plan.error_code, LEGACY_MIX_ADDITEM_ERR_ITEM_MISMATCH);
    EXPECT_TRUE(plan.send_nack);

    RecordingSink sink;
    (void)apply_item_mix_add_item_side_effects(plan, sink);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"nack"}));
    EXPECT_EQ(sink.last_error_code,
              LEGACY_MIX_ADDITEM_ERR_ITEM_MISMATCH);
}

TEST(ApplyItemMixAddItemSideEffects, SlotLockedEmitsNackCode3) {
    auto in = PassingGates();
    in.slot_is_locked = true;
    auto plan = item_mix_add_item_side_effect_plan(in, 3);
    EXPECT_EQ(plan.error_code, LEGACY_MIX_ADDITEM_ERR_SLOT_LOCKED);

    RecordingSink sink;
    (void)apply_item_mix_add_item_side_effects(plan, sink);
    EXPECT_EQ(sink.last_error_code, LEGACY_MIX_ADDITEM_ERR_SLOT_LOCKED);
}

TEST(ApplyItemMixAddItemSideEffects, OptionItemEmitsNackCode4) {
    auto in = PassingGates();
    in.is_option_item = true;
    auto plan = item_mix_add_item_side_effect_plan(in, 3);
    EXPECT_EQ(plan.error_code, LEGACY_MIX_ADDITEM_ERR_OPTION_ITEM);

    RecordingSink sink;
    (void)apply_item_mix_add_item_side_effects(plan, sink);
    EXPECT_EQ(sink.last_error_code, LEGACY_MIX_ADDITEM_ERR_OPTION_ITEM);
}

TEST(ApplyItemMixAddItemSideEffects, NoMixInfoEmitsNackCode5) {
    auto in = PassingGates();
    in.has_mix_info = false;
    auto plan = item_mix_add_item_side_effect_plan(in, 3);
    EXPECT_EQ(plan.error_code, LEGACY_MIX_ADDITEM_ERR_NO_MIX_INFO);

    RecordingSink sink;
    (void)apply_item_mix_add_item_side_effects(plan, sink);
    EXPECT_EQ(sink.last_error_code, LEGACY_MIX_ADDITEM_ERR_NO_MIX_INFO);
}

TEST(ApplyItemMixAddItemSideEffects, NotMixableEmitsNackCode6) {
    auto in = PassingGates();
    in.item_kind = 0;  // neither YOUNGYAK nor JEWEL
    in.durability = 2;
    auto plan = item_mix_add_item_side_effect_plan(in, 3);
    EXPECT_EQ(plan.error_code, LEGACY_MIX_ADDITEM_ERR_NOT_MIXABLE);

    RecordingSink sink;
    (void)apply_item_mix_add_item_side_effects(plan, sink);
    EXPECT_EQ(sink.last_error_code, LEGACY_MIX_ADDITEM_ERR_NOT_MIXABLE);

    // Durability boundary: kind not special but Durability == 1
    // still passes the gate (legacy condition is Durability > 1).
    auto in2 = PassingGates();
    in2.item_kind = 0;
    in2.durability = 1;
    auto ok = item_mix_add_item_side_effect_plan(in2, 3);
    EXPECT_TRUE(ok.send_ack);

    // Jewel kind is exempt regardless of durability.
    auto in3 = PassingGates();
    in3.item_kind = LEGACY_EKIND_JEWEL;
    in3.durability = 100;
    auto ok3 = item_mix_add_item_side_effect_plan(in3, 3);
    EXPECT_TRUE(ok3.send_ack);
}

TEST(ApplyItemMixAddItemSideEffects, GatePrecedenceLocked) {
    // The first failing gate in legacy order wins: not-in-inventory
    // outranks slot-locked.
    auto in = PassingGates();
    in.table_idx_position = 99;
    in.slot_is_locked = true;
    auto plan = item_mix_add_item_side_effect_plan(in, 3);
    EXPECT_EQ(plan.error_code, LEGACY_MIX_ADDITEM_ERR_NOT_IN_INVEN);

    // slot-locked outranks option-item.
    auto in2 = PassingGates();
    in2.slot_is_locked = true;
    in2.is_option_item = true;
    auto plan2 = item_mix_add_item_side_effect_plan(in2, 3);
    EXPECT_EQ(plan2.error_code, LEGACY_MIX_ADDITEM_ERR_SLOT_LOCKED);

    // option-item outranks no-mix-info.
    auto in3 = PassingGates();
    in3.is_option_item = true;
    in3.has_mix_info = false;
    auto plan3 = item_mix_add_item_side_effect_plan(in3, 3);
    EXPECT_EQ(plan3.error_code, LEGACY_MIX_ADDITEM_ERR_OPTION_ITEM);

    // no-mix-info outranks not-mixable.
    auto in4 = PassingGates();
    in4.has_mix_info = false;
    in4.item_kind = 0;
    in4.durability = 5;
    auto plan4 = item_mix_add_item_side_effect_plan(in4, 3);
    EXPECT_EQ(plan4.error_code, LEGACY_MIX_ADDITEM_ERR_NO_MIX_INFO);
}

TEST(ApplyItemMixAddItemSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ItemMixAddItemSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_item_mix_add_item_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.slot_locks, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}
