// char_change_side_effect_runtime_test.cpp
//
// Verifies apply_char_change_side_effects() (the runtime orchestrator
// for the CItemManager::MP_ITEM_SHOPITEM_CHARCHANGE_SYN side-effect
// chain) walks the data-plane plan and dispatches each entry: the
// 7-step success chain in legacy order / the 6-way gate NACK.

#include <mxh/server/char_change_side_effect.hpp>
#include <mxh/server/char_change_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::CharChangeIcon;
using mxh::server::CharChangeSideEffectKind;
using mxh::server::CharChangeSideEffectSink;
using mxh::server::CharChangeValidationFields;
using mxh::server::CharChangeValidationInput;
using mxh::server::apply_char_change_side_effects;
using mxh::server::char_change_side_effect_plan;

class RecordingSink final : public CharChangeSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_player_id = 0;
    std::uint32_t last_nack_code = 0;
    std::uint16_t last_shop_item_idx = 0;
    std::uint16_t last_shop_item_pos = 0;
    CharChangeIcon last_icon_kind = CharChangeIcon::CharChange;
    CharChangeValidationFields last_info{};
    std::uint8_t last_saved_gender = 0;
    float last_saved_height = 0.0f;
    float last_saved_width = 0.0f;
    std::size_t nack_count = 0;

    void send_nack_to_player(std::uint32_t player_id,
                             std::uint32_t nack_code) override {
        calls.push_back("nack");
        last_player_id = player_id;
        last_nack_code = nack_code;
        ++nack_count;
    }
    void send_use_ack_to_player(std::uint32_t player_id,
                                std::uint16_t shop_item_idx,
                                std::uint16_t shop_item_pos) override {
        calls.push_back("useack");
        last_player_id = player_id;
        last_shop_item_idx = shop_item_idx;
        last_shop_item_pos = shop_item_pos;
    }
    void discard_char_change_item(std::uint32_t player_id,
                                  std::uint16_t shop_item_idx,
                                  std::uint16_t shop_item_pos) override {
        calls.push_back("discard");
        last_player_id = player_id;
        last_shop_item_idx = shop_item_idx;
        last_shop_item_pos = shop_item_pos;
    }
    void set_char_change_info(
        std::uint32_t player_id,
        const CharChangeValidationFields& info) override {
        calls.push_back("setinfo");
        last_player_id = player_id;
        last_info = info;
    }
    void broadcast_char_change(
        std::uint32_t player_id, CharChangeIcon icon_kind,
        const CharChangeValidationFields& info,
        std::uint8_t saved_gender, float saved_height,
        float saved_width) override {
        calls.push_back("bcast");
        last_player_id = player_id;
        last_icon_kind = icon_kind;
        last_info = info;
        last_saved_gender = saved_gender;
        last_saved_height = saved_height;
        last_saved_width = saved_width;
    }
    void character_change_info_to_db(
        std::uint32_t player_id, CharChangeIcon icon_kind,
        const CharChangeValidationFields& info,
        std::uint8_t saved_gender, float saved_height,
        float saved_width) override {
        calls.push_back("db");
        last_player_id = player_id;
        last_icon_kind = icon_kind;
        last_info = info;
        last_saved_gender = saved_gender;
        last_saved_height = saved_height;
        last_saved_width = saved_width;
    }
    void send_char_change_ack(std::uint32_t player_id) override {
        calls.push_back("ack");
        last_player_id = player_id;
    }
    void log_item_money(std::uint32_t player_id,
                        std::uint16_t shop_item_idx,
                        std::uint16_t shop_item_pos) override {
        calls.push_back("log");
        last_player_id = player_id;
        last_shop_item_idx = shop_item_idx;
        last_shop_item_pos = shop_item_pos;
    }
};

CharChangeValidationInput PassingGates() {
    CharChangeValidationInput in;
    in.avatar_effect_clear = true;
    in.item_exists = true;
    in.item_icon_is_char_or_shape = true;
    in.height_in_range = true;
    in.width_in_range = true;
    in.gender_in_range = true;
    in.hair_face_in_range = true;
    in.discard_returned_true = true;
    return in;
}

CharChangeValidationFields SampleInfo() {
    CharChangeValidationFields info;
    info.height = 1.05f;
    info.width = 0.95f;
    info.gender = 1;
    info.hair_type = 2;
    info.face_type = 3;
    return info;
}

}  // namespace

TEST(ApplyCharChangeSideEffects, SuccessEmitsSevenStepChainInOrder) {
    auto in = PassingGates();
    auto info = SampleInfo();
    auto plan = char_change_side_effect_plan(
        in, /*player_id=*/0x000C000Du,
        /*shop_item_idx=*/300, /*shop_item_pos=*/4,
        /*icon_kind=*/CharChangeIcon::ShapeChange,
        info, /*saved_gender=*/0,
        /*saved_height=*/1.0f, /*saved_width=*/1.0f);
    EXPECT_TRUE(plan.send_use_ack);
    EXPECT_TRUE(plan.send_char_change_ack);
    EXPECT_TRUE(plan.broadcast);
    EXPECT_TRUE(plan.db_call);
    EXPECT_TRUE(plan.discard_item);
    EXPECT_TRUE(plan.set_char_change_info);
    EXPECT_TRUE(plan.log_item_money);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 7u);
    const CharChangeSideEffectKind expected[] = {
        CharChangeSideEffectKind::DiscardCharChangeItem,
        CharChangeSideEffectKind::SetCharChangeInfo,
        CharChangeSideEffectKind::SendUseAckToPlayer,
        CharChangeSideEffectKind::BroadcastCharChange,
        CharChangeSideEffectKind::CharacterChangeInfoToDB,
        CharChangeSideEffectKind::SendCharChangeAck,
        CharChangeSideEffectKind::LogItemMoney,
    };
    for (std::size_t i = 0u; i < 7u; ++i) {
        EXPECT_EQ(plan.effects[i].kind, expected[i]);
    }
    EXPECT_EQ(plan.effects[3].icon_kind, CharChangeIcon::ShapeChange);
    EXPECT_EQ(plan.effects[3].info.height, 1.05f);
    EXPECT_EQ(plan.effects[3].saved_gender, 0u);

    RecordingSink sink;
    auto out = apply_char_change_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 7u);
    EXPECT_EQ(out.discards, 1u);
    EXPECT_EQ(out.info_sets, 1u);
    EXPECT_EQ(out.use_acks_sent, 1u);
    EXPECT_EQ(out.broadcasts, 1u);
    EXPECT_EQ(out.db_calls, 1u);
    EXPECT_EQ(out.char_acks_sent, 1u);
    EXPECT_EQ(out.money_logs, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.discard_flag_consumed);
    EXPECT_TRUE(out.set_info_flag_consumed);
    EXPECT_TRUE(out.use_ack_flag_consumed);
    EXPECT_TRUE(out.broadcast_flag_consumed);
    EXPECT_TRUE(out.db_flag_consumed);
    EXPECT_TRUE(out.char_ack_flag_consumed);
    EXPECT_TRUE(out.log_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({"discard", "setinfo", "useack",
                                        "bcast", "db", "ack", "log"}));
    EXPECT_EQ(sink.last_player_id, 0x000C000Du);
    EXPECT_EQ(sink.last_icon_kind, CharChangeIcon::ShapeChange);
    EXPECT_EQ(sink.last_info.gender, 1u);
    EXPECT_EQ(sink.last_info.hair_type, 2u);
    EXPECT_EQ(sink.last_info.face_type, 3u);
    EXPECT_EQ(sink.last_info.height, 1.05f);
    EXPECT_EQ(sink.last_info.width, 0.95f);
    EXPECT_EQ(sink.last_saved_gender, 0u);
    EXPECT_EQ(sink.last_saved_height, 1.0f);
    EXPECT_EQ(sink.last_saved_width, 1.0f);
    EXPECT_EQ(sink.last_shop_item_idx, 300u);
    EXPECT_EQ(sink.last_shop_item_pos, 4u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyCharChangeSideEffects, GateNackCodesSweep) {
    struct Case {
        void (*mutate)(CharChangeValidationInput&);
        std::uint32_t expected_code;
    };
    const Case cases[] = {
        {[](CharChangeValidationInput& i) { i.avatar_effect_clear = false; }, 6u},
        {[](CharChangeValidationInput& i) { i.item_exists = false; }, 1u},
        {[](CharChangeValidationInput& i) { i.item_icon_is_char_or_shape = false; }, 1u},
        {[](CharChangeValidationInput& i) { i.height_in_range = false; }, 2u},
        {[](CharChangeValidationInput& i) { i.width_in_range = false; }, 2u},
        {[](CharChangeValidationInput& i) { i.gender_in_range = false; }, 3u},
        {[](CharChangeValidationInput& i) { i.hair_face_in_range = false; }, 4u},
        {[](CharChangeValidationInput& i) { i.discard_returned_true = false; }, 5u},
    };
    for (const auto& c : cases) {
        auto in = PassingGates();
        c.mutate(in);
        auto plan = char_change_side_effect_plan(
            in, 7, 1, 2, CharChangeIcon::CharChange,
            SampleInfo(), 0, 1.0f, 1.0f);
        EXPECT_TRUE(plan.send_nack);
        EXPECT_EQ(plan.nack_code, c.expected_code);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  CharChangeSideEffectKind::SendNackToPlayer);
        EXPECT_EQ(plan.effects[0].nack_code, c.expected_code);

        RecordingSink sink;
        (void)apply_char_change_side_effects(plan, sink);
        EXPECT_EQ(sink.last_nack_code, c.expected_code);
    }
}

TEST(ApplyCharChangeSideEffects, GatePrecedenceLocked) {
    // Avatar effect outranks every other gate.
    auto in = PassingGates();
    in.avatar_effect_clear = false;
    in.item_exists = false;
    auto plan = char_change_side_effect_plan(
        in, 7, 1, 2, CharChangeIcon::CharChange,
        SampleInfo(), 0, 1.0f, 1.0f);
    EXPECT_EQ(plan.nack_code, 6u);

    // Bad item outranks bad shape.
    auto in2 = PassingGates();
    in2.item_exists = false;
    in2.height_in_range = false;
    auto plan2 = char_change_side_effect_plan(
        in2, 7, 1, 2, CharChangeIcon::CharChange,
        SampleInfo(), 0, 1.0f, 1.0f);
    EXPECT_EQ(plan2.nack_code, 1u);

    // Bad shape outranks bad gender; bad gender outranks bad hair.
    auto in3 = PassingGates();
    in3.width_in_range = false;
    in3.gender_in_range = false;
    in3.hair_face_in_range = false;
    auto plan3 = char_change_side_effect_plan(
        in3, 7, 1, 2, CharChangeIcon::CharChange,
        SampleInfo(), 0, 1.0f, 1.0f);
    EXPECT_EQ(plan3.nack_code, 2u);
}

TEST(ApplyCharChangeSideEffects, EmptyPlanIsNoOp) {
    mxh::server::CharChangeSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_char_change_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.use_acks_sent, 0u);
    EXPECT_EQ(out.discards, 0u);
    EXPECT_EQ(out.info_sets, 0u);
    EXPECT_EQ(out.broadcasts, 0u);
    EXPECT_EQ(out.db_calls, 0u);
    EXPECT_EQ(out.char_acks_sent, 0u);
    EXPECT_EQ(out.money_logs, 0u);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.use_ack_flag_consumed);
    EXPECT_FALSE(out.discard_flag_consumed);
    EXPECT_FALSE(out.set_info_flag_consumed);
    EXPECT_FALSE(out.broadcast_flag_consumed);
    EXPECT_FALSE(out.db_flag_consumed);
    EXPECT_FALSE(out.char_ack_flag_consumed);
    EXPECT_FALSE(out.log_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}
