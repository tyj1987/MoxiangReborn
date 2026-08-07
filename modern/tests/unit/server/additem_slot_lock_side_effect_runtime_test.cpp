// additem_slot_lock_side_effect_runtime_test.cpp
//
// Verifies apply_additem_slot_lock_side_effects() (the runtime
// orchestrator for the MP_ITEMEXT_*_ADDITEM_SYN slot-lock chain) walks
// the data-plane plan and dispatches each entry: lock-then-ACK on
// success / 3-way gate NACK / empty plan when the player is missing.

#include <mxh/server/additem_slot_lock_side_effect.hpp>
#include <mxh/server/additem_slot_lock_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::AddItemSlotLockSideEffectKind;
using mxh::server::AddItemSlotLockSideEffectSink;
using mxh::server::AddItemSlotLockValidationInput;
using mxh::server::apply_additem_slot_lock_side_effects;
using mxh::server::additem_slot_lock_side_effect_plan;

class RecordingSink final : public AddItemSlotLockSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_player_id = 0;
    std::uint16_t last_position = 0;
    std::uint16_t last_w_icon_idx = 0;
    std::uint8_t last_error_code = 0;
    std::size_t ack_count = 0;
    std::size_t nack_count = 0;
    std::size_t lock_count = 0;

    void send_ack_to_player(std::uint32_t player_id,
                            std::uint16_t position,
                            std::uint16_t w_icon_idx) override {
        calls.push_back("ack");
        last_player_id = player_id;
        last_position = position;
        last_w_icon_idx = w_icon_idx;
        ++ack_count;
    }
    void send_nack_to_player(std::uint32_t player_id,
                             std::uint16_t position,
                             std::uint8_t error_code) override {
        calls.push_back("nack");
        last_player_id = player_id;
        last_position = position;
        last_error_code = error_code;
        ++nack_count;
    }
    void set_slot_lock(std::uint32_t player_id,
                       std::uint16_t position) override {
        calls.push_back("lock");
        last_player_id = player_id;
        last_position = position;
        ++lock_count;
    }
};

}  // namespace

TEST(ApplyAddItemSlotLockSideEffects, SuccessEmitsLockThenAckInOrder) {
    AddItemSlotLockValidationInput in;
    in.player_found = true;
    in.position_in_inventory = true;
    in.item_of_check_passed = true;
    in.slot_unlocked = true;
    auto plan = additem_slot_lock_side_effect_plan(
        in, /*player_id=*/0x00140015u,
        /*position=*/9, /*w_icon_idx=*/123);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_TRUE(plan.set_lock);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              AddItemSlotLockSideEffectKind::SetSlotLock);
    EXPECT_EQ(plan.effects[1].kind,
              AddItemSlotLockSideEffectKind::SendAckToPlayer);
    EXPECT_EQ(plan.effects[1].w_icon_idx, 123u);

    RecordingSink sink;
    auto out = apply_additem_slot_lock_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 2u);
    EXPECT_EQ(out.slot_locks, 1u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.lock_flag_consumed);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"lock", "ack"}));
    EXPECT_EQ(sink.last_player_id, 0x00140015u);
    EXPECT_EQ(sink.last_position, 9u);
    EXPECT_EQ(sink.last_w_icon_idx, 123u);
}

TEST(ApplyAddItemSlotLockSideEffects, GateErrorCodesSweep) {
    struct Case {
        void (*mutate)(AddItemSlotLockValidationInput&);
        std::uint8_t expected_code;
    };
    const Case cases[] = {
        {[](AddItemSlotLockValidationInput& i) { i.position_in_inventory = false; }, 1u},
        {[](AddItemSlotLockValidationInput& i) { i.item_of_check_passed = false; }, 2u},
        {[](AddItemSlotLockValidationInput& i) { i.slot_unlocked = false; }, 3u},
    };
    for (const auto& c : cases) {
        auto in = AddItemSlotLockValidationInput{};
        in.player_found = true;
        in.position_in_inventory = true;
        in.item_of_check_passed = true;
        in.slot_unlocked = true;
        c.mutate(in);
        auto plan = additem_slot_lock_side_effect_plan(
            in, 7, 3, 4);
        EXPECT_TRUE(plan.send_nack);
        EXPECT_EQ(plan.error_code, c.expected_code);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  AddItemSlotLockSideEffectKind::SendNackToPlayer);
        EXPECT_EQ(plan.effects[0].error_code, c.expected_code);

        RecordingSink sink;
        (void)apply_additem_slot_lock_side_effects(plan, sink);
        EXPECT_EQ(sink.last_error_code, c.expected_code);
    }
}

TEST(ApplyAddItemSlotLockSideEffects, NoPlayerEmitsEmptyPlan) {
    AddItemSlotLockValidationInput in;
    in.player_found = false;
    in.position_in_inventory = true;
    in.item_of_check_passed = true;
    in.slot_unlocked = true;
    auto plan = additem_slot_lock_side_effect_plan(
        in, 7, 3, 4);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_additem_slot_lock_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyAddItemSlotLockSideEffects, GatePrecedenceLocked) {
    // Wrong table outranks item-of failure.
    auto in = AddItemSlotLockValidationInput{};
    in.player_found = true;
    in.position_in_inventory = false;
    in.item_of_check_passed = false;
    auto plan = additem_slot_lock_side_effect_plan(
        in, 7, 3, 4);
    EXPECT_EQ(plan.error_code, 1u);

    // Item-of failure outranks already-locked.
    auto in2 = AddItemSlotLockValidationInput{};
    in2.player_found = true;
    in2.position_in_inventory = true;
    in2.item_of_check_passed = false;
    in2.slot_unlocked = false;
    auto plan2 = additem_slot_lock_side_effect_plan(
        in2, 7, 3, 4);
    EXPECT_EQ(plan2.error_code, 2u);
}

TEST(ApplyAddItemSlotLockSideEffects, EmptyPlanIsNoOp) {
    mxh::server::AddItemSlotLockSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_additem_slot_lock_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.slot_locks, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.lock_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}
