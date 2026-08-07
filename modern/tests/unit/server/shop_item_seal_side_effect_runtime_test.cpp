// shop_item_seal_side_effect_runtime_test.cpp
//
// Verifies apply_shop_item_seal_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_SHOPITEM_SEAL_SYN
// side-effect chain) walks the data-plane plan and dispatches the
// 8-step success chain in legacy order, the single NACK for each of
// the 8 failure gates, or stays a no-op when the player is missing.

#include <mxh/server/shop_item_seal_side_effect.hpp>
#include <mxh/server/shop_item_seal_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::ShopItemSealSideEffectKind;
using mxh::server::ShopItemSealSideEffectSink;
using mxh::server::apply_shop_item_seal_side_effects;
using mxh::server::shop_item_seal_side_effect_plan;

class RecordingSink final : public ShopItemSealSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_target_db_idx = 0;
    std::uint32_t last_target_item_param = 0;
    std::uint16_t last_seal_item_idx = 0;
    std::uint16_t last_seal_item_pos = 0;
    std::uint32_t last_nack_code = 0;

    void log_shop_item_use() override {
        calls.push_back("log_use");
    }
    void set_item_param_seal(std::uint32_t target_db_idx,
                             std::uint32_t target_item_param) override {
        calls.push_back("param_seal");
        last_target_db_idx = target_db_idx;
        last_target_item_param = target_item_param;
    }
    void delete_using_shop_item_info() override {
        calls.push_back("del_use");
    }
    void shop_item_param_update_to_db(
        std::uint32_t target_db_idx,
        std::uint32_t target_item_param) override {
        calls.push_back("db_param");
        last_target_db_idx = target_db_idx;
        last_target_item_param = target_item_param;
    }
    void shop_item_delete_to_db(std::uint32_t target_db_idx) override {
        calls.push_back("db_del");
        last_target_db_idx = target_db_idx;
    }
    void log_shop_item_seal() override {
        calls.push_back("log_seal");
    }
    void broadcast_use_ack(std::uint16_t seal_item_idx,
                           std::uint16_t seal_item_pos) override {
        calls.push_back("use_ack");
        last_seal_item_idx = seal_item_idx;
        last_seal_item_pos = seal_item_pos;
    }
    void broadcast_seal_ack() override {
        calls.push_back("seal_ack");
    }
    void broadcast_seal_nack(std::uint16_t seal_item_idx,
                             std::uint16_t seal_item_pos,
                             std::uint32_t nack_code) override {
        calls.push_back("seal_nack");
        last_seal_item_idx = seal_item_idx;
        last_seal_item_pos = seal_item_pos;
        last_nack_code = nack_code;
    }
};

}  // namespace

TEST(ApplyShopItemSealSideEffects, SuccessEmitsFullChainInOrder) {
    mxh::server::ShopItemSealValidationInput in;
    in.player_found = true;
    in.seal_item_usable = true;
    in.target_item_usable = true;
    in.seal_item_resolved = true;
    in.target_item_resolved = true;
    in.target_item_info_resolved = true;
    in.seal_is_item_seal_kind = true;
    in.target_kind_ok = true;
    in.target_sell_price_forever = true;
    in.target_already_sealed = false;
    in.discard_rt = 0;
    auto plan = shop_item_seal_side_effect_plan(
        in, /*target_db_idx=*/0x00010002u,
        /*seal_item_idx=*/100, /*seal_item_pos=*/7);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 8u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemSealSideEffectKind::LogShopItemUse);
    EXPECT_EQ(plan.effects[7].kind,
              ShopItemSealSideEffectKind::BroadcastSealAck);

    RecordingSink sink;
    auto out = apply_shop_item_seal_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 8u);
    EXPECT_EQ(out.logs_use, 1u);
    EXPECT_EQ(out.param_seals, 1u);
    EXPECT_EQ(out.use_deletes, 1u);
    EXPECT_EQ(out.db_param_updates, 1u);
    EXPECT_EQ(out.db_deletes, 1u);
    EXPECT_EQ(out.logs_seal, 1u);
    EXPECT_EQ(out.use_acks, 1u);
    EXPECT_EQ(out.seal_acks, 1u);
    EXPECT_EQ(out.seal_nacks, 0u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    ASSERT_EQ(sink.calls.size(), 8u);
    EXPECT_EQ(sink.calls[0], "log_use");
    EXPECT_EQ(sink.calls[1], "param_seal");
    EXPECT_EQ(sink.calls[2], "del_use");
    EXPECT_EQ(sink.calls[3], "db_param");
    EXPECT_EQ(sink.calls[4], "db_del");
    EXPECT_EQ(sink.calls[5], "log_seal");
    EXPECT_EQ(sink.calls[6], "use_ack");
    EXPECT_EQ(sink.calls[7], "seal_ack");
    EXPECT_EQ(sink.last_target_db_idx, 0x00010002u);
    EXPECT_EQ(sink.last_target_item_param,
              mxh::server::LEGACY_ITEM_PARAM_SEAL);
    EXPECT_EQ(sink.last_seal_item_idx, 100u);
    EXPECT_EQ(sink.last_seal_item_pos, 7u);
}

TEST(ApplyShopItemSealSideEffects, EachGateEmitsMatchingNackCode) {
    struct Case {
        bool seal_usable, target_usable, seal_resolved, target_resolved;
        bool info_resolved, is_seal_kind, kind_ok, forever, sealed;
        int discard_rt;
        std::uint32_t nack_code;
    };
    const Case cases[] = {
        {false, true,  true,  true,  true, true, true, true, false, 0,
         mxh::server::LEGACY_SEAL_NACK_NOT_USABLE_SEAL},
        {true,  false, true,  true,  true, true, true, true, false, 0,
         mxh::server::LEGACY_SEAL_NACK_NOT_USABLE_TARGET},
        {true,  true,  false, true,  true, true, true, true, false, 0,
         mxh::server::LEGACY_SEAL_NACK_NOT_FOUND},
        {true,  true,  true,  true,  true, false, true, true, false, 0,
         mxh::server::LEGACY_SEAL_NACK_WRONG_SEAL_ITEM},
        {true,  true,  true,  true,  true, true, false, true, false, 0,
         mxh::server::LEGACY_SEAL_NACK_WRONG_KIND},
        {true,  true,  true,  true,  true, true, true, false, false, 0,
         mxh::server::LEGACY_SEAL_NACK_NOT_FOREVER},
        {true,  true,  true,  true,  true, true, true, true, true, 0,
         mxh::server::LEGACY_SEAL_NACK_ALREADY_SEALED},
        {true,  true,  true,  true,  true, true, true, true, false, 5,
         mxh::server::LEGACY_SEAL_NACK_DISCARD_FAIL},
    };
    for (const auto& c : cases) {
        mxh::server::ShopItemSealValidationInput in;
        in.player_found = true;
        in.seal_item_usable = c.seal_usable;
        in.target_item_usable = c.target_usable;
        in.seal_item_resolved = c.seal_resolved;
        in.target_item_resolved = c.target_resolved;
        in.target_item_info_resolved = c.info_resolved;
        in.seal_is_item_seal_kind = c.is_seal_kind;
        in.target_kind_ok = c.kind_ok;
        in.target_sell_price_forever = c.forever;
        in.target_already_sealed = c.sealed;
        in.discard_rt = c.discard_rt;
        auto plan = shop_item_seal_side_effect_plan(in, 1, 2, 3);
        EXPECT_TRUE(plan.send_nack);
        EXPECT_FALSE(plan.send_ack);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  ShopItemSealSideEffectKind::BroadcastSealNack);
        EXPECT_EQ(plan.effects[0].nack_code, c.nack_code);

        RecordingSink sink;
        auto out = apply_shop_item_seal_side_effects(plan, sink);
        EXPECT_EQ(out.seal_nacks, 1u);
        EXPECT_TRUE(out.nack_flag_consumed);
        EXPECT_FALSE(out.ack_flag_consumed);
        EXPECT_EQ(sink.last_nack_code, c.nack_code);
        EXPECT_EQ(sink.last_seal_item_idx, 2u);
        EXPECT_EQ(sink.last_seal_item_pos, 3u);
    }
}

TEST(ApplyShopItemSealSideEffects, NoPlayerIsNoOp) {
    mxh::server::ShopItemSealValidationInput in;
    in.player_found = false;
    auto plan = shop_item_seal_side_effect_plan(in, 1, 2, 3);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_shop_item_seal_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.seal_nacks, 0u);
    EXPECT_EQ(out.seal_acks, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyShopItemSealSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ShopItemSealSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_shop_item_seal_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.logs_use, 0u);
    EXPECT_EQ(out.param_seals, 0u);
    EXPECT_EQ(out.use_deletes, 0u);
    EXPECT_EQ(out.db_param_updates, 0u);
    EXPECT_EQ(out.db_deletes, 0u);
    EXPECT_EQ(out.logs_seal, 0u);
    EXPECT_EQ(out.use_acks, 0u);
    EXPECT_EQ(out.seal_acks, 0u);
    EXPECT_EQ(out.seal_nacks, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyShopItemSealSideEffects, NoPlayerOverridesAllGates) {
    mxh::server::ShopItemSealValidationInput in;
    in.player_found = false;
    in.seal_item_usable = false;
    in.target_item_usable = false;
    in.discard_rt = 9;
    auto plan = shop_item_seal_side_effect_plan(in, 1, 2, 3);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    (void)apply_shop_item_seal_side_effects(plan, sink);
    EXPECT_TRUE(sink.calls.empty());
}
