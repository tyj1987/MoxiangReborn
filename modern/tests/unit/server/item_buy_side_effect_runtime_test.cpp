// item_buy_side_effect_runtime_test.cpp
//
// Verifies apply_item_buy_side_effects() (the runtime orchestrator
// for the CItemManager::MP_ITEM_BUY_SYN side-effect chain) walks the
// data-plane plan and dispatches the single entry to its subsystem:
// NACK with the gate-specific ECode on failure / silent success on
// BuyItem rt==0.

#include <mxh/server/item_buy_side_effect.hpp>
#include <mxh/server/item_buy_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::ItemBuySideEffectKind;
using mxh::server::ItemBuySideEffectSink;
using mxh::server::LEGACY_NO_DEMANDITEM_BUY;
using mxh::server::LEGACY_NOT_EXIST_BUY;
using mxh::server::apply_item_buy_side_effects;
using mxh::server::item_buy_side_effect_plan;

class RecordingSink final : public ItemBuySideEffectSink {
public:
    std::string last_call;
    int last_rt = 0;
    int last_ecode = 0;
    std::uint16_t last_buy_item_idx = 0;
    std::uint16_t last_buy_item_num = 0;
    std::uint16_t last_dealer_idx = 0;
    std::size_t nack_count = 0;
    std::size_t silent_count = 0;

    void broadcast_buy_nack(std::uint16_t buy_item_idx,
                            std::uint16_t buy_item_num,
                            std::uint16_t dealer_idx,
                            int original_rt,
                            int ecode) override {
        last_call = "nack";
        last_buy_item_idx = buy_item_idx;
        last_buy_item_num = buy_item_num;
        last_dealer_idx = dealer_idx;
        last_rt = original_rt;
        last_ecode = ecode;
        ++nack_count;
    }
    void silent_success() override {
        last_call = "silent";
        ++silent_count;
    }
};

}  // namespace

TEST(ApplyItemBuySideEffects, NpcGateFailureEmitsNackNotExist) {
    // Legacy: CheckHackNpc failure short-circuits before the other
    // gates; ECode = NOT_EXIST, no BuyItem call (rt sentinel -1).
    auto plan = item_buy_side_effect_plan(
        /*buy_rt=*/0, /*npc_gate_ok=*/false, /*demand_ok=*/true,
        /*buy_item_idx=*/100, /*buy_item_num=*/1, /*dealer_idx=*/50);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.silent_success);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemBuySideEffectKind::BroadcastNackNpcGate);
    EXPECT_EQ(plan.effects[0].ecode, LEGACY_NOT_EXIST_BUY);

    RecordingSink sink;
    auto out = apply_item_buy_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_EQ(out.silent_successes, 0u);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_FALSE(out.silent_flag_consumed);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_ecode, LEGACY_NOT_EXIST_BUY);
    EXPECT_EQ(sink.last_rt, -1);
    EXPECT_EQ(sink.nack_count, 1u);
    EXPECT_EQ(sink.silent_count, 0u);
}

TEST(ApplyItemBuySideEffects, DemandFailureEmitsNackNoDemandItem) {
    auto plan = item_buy_side_effect_plan(
        /*buy_rt=*/0, /*npc_gate_ok=*/true, /*demand_ok=*/false,
        /*buy_item_idx=*/100, /*buy_item_num=*/1, /*dealer_idx=*/50);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.silent_success);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemBuySideEffectKind::BroadcastNackDemand);
    EXPECT_EQ(plan.effects[0].ecode, LEGACY_NO_DEMANDITEM_BUY);

    RecordingSink sink;
    auto out = apply_item_buy_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_ecode, LEGACY_NO_DEMANDITEM_BUY);
    EXPECT_EQ(sink.last_rt, -1);
    EXPECT_EQ(sink.nack_count, 1u);
    EXPECT_EQ(sink.silent_count, 0u);
}

TEST(ApplyItemBuySideEffects, SuccessIsSilentNoWire) {
    // Legacy: BuyItem rt == 0 -> empty body; ObtainItemEx emits its
    // own ACK.
    auto plan = item_buy_side_effect_plan(
        /*buy_rt=*/0, /*npc_gate_ok=*/true, /*demand_ok=*/true,
        /*buy_item_idx=*/100, /*buy_item_num=*/1, /*dealer_idx=*/50);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.silent_success);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_item_buy_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.silent_successes, 1u);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(out.silent_flag_consumed);
    EXPECT_EQ(sink.last_call, "silent");
    EXPECT_EQ(sink.silent_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyItemBuySideEffects, BuyFailureEmitsNackWithRt) {
    // Legacy: BuyItem non-zero -> MP_ITEM_BUY_NACK with ECode = rt.
    auto plan = item_buy_side_effect_plan(
        /*buy_rt=*/7, /*npc_gate_ok=*/true, /*demand_ok=*/true,
        /*buy_item_idx=*/100, /*buy_item_num=*/1, /*dealer_idx=*/50);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.silent_success);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemBuySideEffectKind::BroadcastNackBuyFail);
    EXPECT_EQ(plan.effects[0].ecode, 7);

    RecordingSink sink;
    auto out = apply_item_buy_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_rt, 7);
    EXPECT_EQ(sink.last_ecode, 7);
}

TEST(ApplyItemBuySideEffects, EmptyPlanIsNoOp) {
    mxh::server::ItemBuySideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_item_buy_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.silent_successes, 0u);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.silent_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.nack_count, 0u);
    EXPECT_EQ(sink.silent_count, 0u);
}

TEST(ApplyItemBuySideEffects, GatePrecedenceNpcOverDemandOverRt) {
    // NpcGate wins over Demand and over rt.
    auto npc_plan = item_buy_side_effect_plan(
        3, /*npc_gate_ok=*/false, /*demand_ok=*/false, 1, 2, 3);
    EXPECT_EQ(npc_plan.effects[0].ecode, LEGACY_NOT_EXIST_BUY);
    EXPECT_EQ(npc_plan.effects[0].kind,
              ItemBuySideEffectKind::BroadcastNackNpcGate);

    // Demand wins over rt (but not over NpcGate).
    auto demand_plan = item_buy_side_effect_plan(
        3, /*npc_gate_ok=*/true, /*demand_ok=*/false, 1, 2, 3);
    EXPECT_EQ(demand_plan.effects[0].ecode, LEGACY_NO_DEMANDITEM_BUY);
    EXPECT_EQ(demand_plan.effects[0].kind,
              ItemBuySideEffectKind::BroadcastNackDemand);

    RecordingSink sink;
    (void)apply_item_buy_side_effects(npc_plan, sink);
    EXPECT_EQ(sink.last_ecode, LEGACY_NOT_EXIST_BUY);
    (void)apply_item_buy_side_effects(demand_plan, sink);
    EXPECT_EQ(sink.last_ecode, LEGACY_NO_DEMANDITEM_BUY);
}

TEST(ApplyItemBuySideEffects, VariousBuyFailureCodesAllEmitNack) {
    for (int rt : {1, 5, 99, 100, -1}) {
        auto plan = item_buy_side_effect_plan(
            rt, /*npc_gate_ok=*/true, /*demand_ok=*/true, 1, 2, 3);
        EXPECT_TRUE(plan.send_nack);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  ItemBuySideEffectKind::BroadcastNackBuyFail);
        EXPECT_EQ(plan.effects[0].ecode, rt);

        RecordingSink sink;
        (void)apply_item_buy_side_effects(plan, sink);
        EXPECT_EQ(sink.last_call, "nack");
        EXPECT_EQ(sink.last_rt, rt);
        EXPECT_EQ(sink.last_ecode, rt);
    }
}

TEST(ApplyItemBuySideEffects, FieldPassthroughOnNack) {
    auto plan = item_buy_side_effect_plan(
        9, /*npc_gate_ok=*/true, /*demand_ok=*/true,
        /*buy_item_idx=*/21, /*buy_item_num=*/22, /*dealer_idx=*/23);
    RecordingSink sink;
    (void)apply_item_buy_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_buy_item_idx, 21u);
    EXPECT_EQ(sink.last_buy_item_num, 22u);
    EXPECT_EQ(sink.last_dealer_idx, 23u);
    EXPECT_EQ(sink.last_rt, 9);
    EXPECT_EQ(sink.last_ecode, 9);
}
