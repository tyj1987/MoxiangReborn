// Tests for the generic AddItem slot-lock side-effect dispatcher.

#include <mxh/server/additem_slot_lock_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

AddItemSlotLockValidationInput success_input() {
    AddItemSlotLockValidationInput in{};
    in.player_found = true;
    in.position_in_inventory = true;
    in.item_of_check_passed = true;
    in.slot_unlocked = true;
    return in;
}

TEST(AddItemSlotLockOutcome, AllGatesPassIsAck) {
    EXPECT_EQ(classify_additem_slot_lock_outcome(success_input()),
              AddItemSlotLockOutcome::Ack);
}

TEST(AddItemSlotLockOutcome, PlayerMissingIsNoPlayer) {
    auto in = success_input();
    in.player_found = false;
    EXPECT_EQ(classify_additem_slot_lock_outcome(in),
              AddItemSlotLockOutcome::NoPlayer);
}

TEST(AddItemSlotLockOutcome, WrongTableIsNackCode1) {
    auto in = success_input();
    in.position_in_inventory = false;
    EXPECT_EQ(classify_additem_slot_lock_outcome(in),
              AddItemSlotLockOutcome::Nack);
    EXPECT_EQ(additem_slot_lock_error_code(in),
              AddItemSlotLockError::WrongTable);
}

TEST(AddItemSlotLockOutcome, ItemOfFailedIsNackCode2) {
    auto in = success_input();
    in.item_of_check_passed = false;
    EXPECT_EQ(classify_additem_slot_lock_outcome(in),
              AddItemSlotLockOutcome::Nack);
    EXPECT_EQ(additem_slot_lock_error_code(in),
              AddItemSlotLockError::ItemOfFailed);
}

TEST(AddItemSlotLockOutcome, SlotLockedIsNackCode3) {
    auto in = success_input();
    in.slot_unlocked = false;
    EXPECT_EQ(classify_additem_slot_lock_outcome(in),
              AddItemSlotLockOutcome::Nack);
    EXPECT_EQ(additem_slot_lock_error_code(in),
              AddItemSlotLockError::SlotAlreadyLocked);
}

TEST(AddItemSlotLockOutcome, WrongTableTakesPrecedence) {
    auto in = success_input();
    in.position_in_inventory = false;
    in.item_of_check_passed = false;
    in.slot_unlocked = false;
    EXPECT_EQ(additem_slot_lock_error_code(in),
              AddItemSlotLockError::WrongTable);
}

TEST(AddItemSlotLockErrorCode, MapsInputsToLegacyCodes) {
    AddItemSlotLockValidationInput in{};
    in.position_in_inventory = true;
    in.item_of_check_passed = true;
    in.slot_unlocked = true;
    EXPECT_EQ(additem_slot_lock_error_code(in),
              AddItemSlotLockError::None);
    in.item_of_check_passed = false;
    EXPECT_EQ(additem_slot_lock_error_code(in),
              AddItemSlotLockError::ItemOfFailed);
    in.position_in_inventory = false;
    in.item_of_check_passed = true;
    EXPECT_EQ(additem_slot_lock_error_code(in),
              AddItemSlotLockError::WrongTable);
    in.position_in_inventory = true;
    in.slot_unlocked = false;
    EXPECT_EQ(additem_slot_lock_error_code(in),
              AddItemSlotLockError::SlotAlreadyLocked);
}

TEST(AddItemSlotLockPlan, AckEmitsSetLockThenAck) {
    auto in = success_input();
    auto plan = additem_slot_lock_side_effect_plan(in, 100, 50, 1234);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_TRUE(plan.set_lock);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_EQ(plan.error_code, 0u);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              AddItemSlotLockSideEffectKind::SetSlotLock);
    EXPECT_EQ(plan.effects[0].position, 50u);
    EXPECT_EQ(plan.effects[1].kind,
              AddItemSlotLockSideEffectKind::SendAckToPlayer);
    EXPECT_EQ(plan.effects[1].w_icon_idx, 1234u);
}

TEST(AddItemSlotLockPlan, WrongTableSendsNackCode1) {
    auto in = success_input();
    in.position_in_inventory = false;
    auto plan = additem_slot_lock_side_effect_plan(in, 100, 50, 1234);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_EQ(plan.error_code, 1u);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              AddItemSlotLockSideEffectKind::SendNackToPlayer);
    EXPECT_EQ(plan.effects[0].error_code, 1u);
}

TEST(AddItemSlotLockPlan, ItemOfFailedSendsNackCode2) {
    auto in = success_input();
    in.item_of_check_passed = false;
    auto plan = additem_slot_lock_side_effect_plan(in, 100, 50, 1234);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.error_code, 2u);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].error_code, 2u);
}

TEST(AddItemSlotLockPlan, SlotLockedSendsNackCode3) {
    auto in = success_input();
    in.slot_unlocked = false;
    auto plan = additem_slot_lock_side_effect_plan(in, 100, 50, 1234);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.error_code, 3u);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].error_code, 3u);
}

TEST(AddItemSlotLockPlan, NoPlayerEmitsEmptyPlan) {
    auto in = success_input();
    in.player_found = false;
    auto plan = additem_slot_lock_side_effect_plan(in, 100, 50, 1234);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_FALSE(plan.set_lock);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AddItemSlotLockPlan, PlanIsIdempotent) {
    auto in = success_input();
    auto a = additem_slot_lock_side_effect_plan(in, 100, 50, 1234);
    auto b = additem_slot_lock_side_effect_plan(in, 100, 50, 1234);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.set_lock, b.set_lock);
    EXPECT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].player_id, b.effects[i].player_id);
    }
}

