// unique_item_mix_side_effect_runtime_test.cpp
//
// Verifies apply_unique_item_mix_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEMEXT_UNIQUEITEM_MIX_SYN
// side-effect chain) walks the data-plane plan and dispatches each
// entry: per-material discard/ACK/log triplets + basic trio + roll +
// obtain / silent empty plan on gate failure.

#include <mxh/server/unique_item_mix_side_effect.hpp>
#include <mxh/server/unique_item_mix_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::UniqueItemMixMaterial;
using mxh::server::UniqueItemMixSideEffectKind;
using mxh::server::UniqueItemMixSideEffectSink;
using mxh::server::UniqueItemMixValidationInput;
using mxh::server::apply_unique_item_mix_side_effects;
using mxh::server::unique_item_mix_side_effect_plan;

class RecordingSink final : public UniqueItemMixSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_player_id = 0;
    UniqueItemMixMaterial last_material{};
    std::uint32_t last_basic_pos = 0;
    std::uint16_t last_basic_w_idx = 0;
    std::uint32_t last_basic_db_idx = 0;
    std::uint32_t last_result_w_idx = 0;
    std::uint16_t last_obtain_num = 0;
    std::size_t obtain_count = 0;

    void discard_material_item(
        std::uint32_t player_id,
        const UniqueItemMixMaterial& material) override {
        calls.push_back("mdisc");
        last_player_id = player_id;
        last_material = material;
    }
    void send_material_delete_ack(
        std::uint32_t player_id,
        const UniqueItemMixMaterial& material) override {
        calls.push_back("mack");
        last_player_id = player_id;
        last_material = material;
    }
    void log_material_discard(
        std::uint32_t player_id,
        const UniqueItemMixMaterial& material) override {
        calls.push_back("mlog");
        last_player_id = player_id;
        last_material = material;
    }
    void discard_basic_item(std::uint32_t player_id,
                            std::uint32_t basic_pos,
                            std::uint16_t basic_w_idx,
                            std::uint32_t basic_db_idx) override {
        calls.push_back("bdisc");
        last_player_id = player_id;
        last_basic_pos = basic_pos;
        last_basic_w_idx = basic_w_idx;
        last_basic_db_idx = basic_db_idx;
    }
    void send_basic_delete_ack(std::uint32_t player_id,
                               std::uint32_t basic_pos,
                               std::uint16_t basic_w_idx) override {
        calls.push_back("back");
        last_player_id = player_id;
        last_basic_pos = basic_pos;
        last_basic_w_idx = basic_w_idx;
    }
    void log_basic_discard(std::uint32_t player_id,
                           std::uint32_t basic_pos,
                           std::uint16_t basic_w_idx,
                           std::uint32_t basic_db_idx) override {
        calls.push_back("blog");
        last_player_id = player_id;
        last_basic_pos = basic_pos;
        last_basic_w_idx = basic_w_idx;
        last_basic_db_idx = basic_db_idx;
    }
    void roll_random_result_item(std::uint32_t player_id,
                                 std::uint32_t result_w_idx) override {
        calls.push_back("roll");
        last_player_id = player_id;
        last_result_w_idx = result_w_idx;
    }
    void obtain_result_item(std::uint32_t player_id,
                            std::uint32_t result_w_idx,
                            std::uint16_t obtain_num) override {
        calls.push_back("obtain");
        last_player_id = player_id;
        last_result_w_idx = result_w_idx;
        last_obtain_num = obtain_num;
        ++obtain_count;
    }
};

UniqueItemMixValidationInput PassingGates() {
    UniqueItemMixValidationInput in;
    in.basic_item_exists = true;
    in.all_materials_exist = true;
    in.mix_info_exists = true;
    in.enough_material_for_each_kind = true;
    in.inventory_has_space = true;
    return in;
}

}  // namespace

TEST(ApplyUniqueItemMixSideEffects, MixedEmitsFullChainWithTwoMaterials) {
    auto in = PassingGates();
    std::vector<UniqueItemMixMaterial> materials;
    materials.push_back(UniqueItemMixMaterial{/*pos=*/1, /*w_icon_idx=*/11,
                                              /*db_idx=*/101, /*dur=*/2});
    materials.push_back(UniqueItemMixMaterial{/*pos=*/3, /*w_icon_idx=*/22,
                                              /*db_idx=*/202, /*dur=*/1});
    auto plan = unique_item_mix_side_effect_plan(
        in, /*player_id=*/0x00180019u, materials,
        /*basic_pos=*/5, /*basic_w_idx=*/33, /*basic_db_idx=*/303,
        /*result_w_idx=*/999, /*obtain_num=*/2);
    EXPECT_TRUE(plan.discard_materials);
    EXPECT_TRUE(plan.discard_basic);
    EXPECT_TRUE(plan.roll_result);
    EXPECT_TRUE(plan.obtain_result);
    EXPECT_TRUE(plan.any_log);
    // 2 materials x 3 + basic trio + roll + obtain = 11 effects.
    ASSERT_EQ(plan.effects.size(), 11u);
    EXPECT_EQ(plan.effects[0].kind,
              UniqueItemMixSideEffectKind::DiscardMaterialItem);
    EXPECT_EQ(plan.effects[1].kind,
              UniqueItemMixSideEffectKind::SendMaterialDeleteAck);
    EXPECT_EQ(plan.effects[2].kind,
              UniqueItemMixSideEffectKind::LogMaterialDiscard);
    EXPECT_EQ(plan.effects[3].kind,
              UniqueItemMixSideEffectKind::DiscardMaterialItem);
    EXPECT_EQ(plan.effects[4].kind,
              UniqueItemMixSideEffectKind::SendMaterialDeleteAck);
    EXPECT_EQ(plan.effects[5].kind,
              UniqueItemMixSideEffectKind::LogMaterialDiscard);
    EXPECT_EQ(plan.effects[6].kind,
              UniqueItemMixSideEffectKind::DiscardBasicItem);
    EXPECT_EQ(plan.effects[7].kind,
              UniqueItemMixSideEffectKind::SendBasicDeleteAck);
    EXPECT_EQ(plan.effects[8].kind,
              UniqueItemMixSideEffectKind::LogBasicDiscard);
    EXPECT_EQ(plan.effects[9].kind,
              UniqueItemMixSideEffectKind::RollRandomResultItem);
    EXPECT_EQ(plan.effects[10].kind,
              UniqueItemMixSideEffectKind::ObtainResultItem);
    EXPECT_EQ(plan.effects[0].material.db_idx, 101u);
    EXPECT_EQ(plan.effects[3].material.pos, 3u);
    EXPECT_EQ(plan.effects[9].result_w_idx, 999u);
    EXPECT_EQ(plan.effects[10].obtain_num, 2u);

    RecordingSink sink;
    auto out = apply_unique_item_mix_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 11u);
    EXPECT_EQ(out.material_discards, 2u);
    EXPECT_EQ(out.material_acks, 2u);
    EXPECT_EQ(out.material_logs, 2u);
    EXPECT_EQ(out.basic_discards, 1u);
    EXPECT_EQ(out.basic_acks, 1u);
    EXPECT_EQ(out.basic_logs, 1u);
    EXPECT_EQ(out.rolls, 1u);
    EXPECT_EQ(out.obtains, 1u);
    EXPECT_TRUE(out.materials_flag_consumed);
    EXPECT_TRUE(out.basic_flag_consumed);
    EXPECT_TRUE(out.roll_flag_consumed);
    EXPECT_TRUE(out.obtain_flag_consumed);
    EXPECT_TRUE(out.log_flag_consumed);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>(
                  {"mdisc", "mack", "mlog", "mdisc", "mack", "mlog",
                   "bdisc", "back", "blog", "roll", "obtain"}));
    EXPECT_EQ(sink.last_player_id, 0x00180019u);
    EXPECT_EQ(sink.last_basic_pos, 5u);
    EXPECT_EQ(sink.last_basic_w_idx, 33u);
    EXPECT_EQ(sink.last_basic_db_idx, 303u);
    EXPECT_EQ(sink.last_result_w_idx, 999u);
    EXPECT_EQ(sink.last_obtain_num, 2u);
    EXPECT_EQ(sink.obtain_count, 1u);
}

TEST(ApplyUniqueItemMixSideEffects, ZeroMaterialsStillEmitsBasicRollObtain) {
    auto in = PassingGates();
    auto plan = unique_item_mix_side_effect_plan(
        in, 7, {}, /*basic_pos=*/5, /*basic_w_idx=*/33,
        /*basic_db_idx=*/303, /*result_w_idx=*/999, /*obtain_num=*/1);
    ASSERT_EQ(plan.effects.size(), 5u);
    EXPECT_EQ(plan.effects[0].kind,
              UniqueItemMixSideEffectKind::DiscardBasicItem);
    EXPECT_EQ(plan.effects[1].kind,
              UniqueItemMixSideEffectKind::SendBasicDeleteAck);
    EXPECT_EQ(plan.effects[2].kind,
              UniqueItemMixSideEffectKind::LogBasicDiscard);
    EXPECT_EQ(plan.effects[3].kind,
              UniqueItemMixSideEffectKind::RollRandomResultItem);
    EXPECT_EQ(plan.effects[4].kind,
              UniqueItemMixSideEffectKind::ObtainResultItem);

    RecordingSink sink;
    auto out = apply_unique_item_mix_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 5u);
    EXPECT_EQ(out.material_discards, 0u);
    EXPECT_EQ(out.basic_discards, 1u);
    EXPECT_EQ(out.rolls, 1u);
    EXPECT_EQ(out.obtains, 1u);
}

TEST(ApplyUniqueItemMixSideEffects, NoSpaceForResultOmitsObtain) {
    auto in = PassingGates();
    in.inventory_has_space = false;
    std::vector<UniqueItemMixMaterial> materials;
    materials.push_back(UniqueItemMixMaterial{1, 11, 101, 2});
    auto plan = unique_item_mix_side_effect_plan(
        in, 7, materials, 5, 33, 303, 999, 0);
    EXPECT_FALSE(plan.obtain_result);
    EXPECT_TRUE(plan.discard_materials);
    EXPECT_TRUE(plan.roll_result);
    // 1 material x 3 + basic trio + roll = 7 effects.
    ASSERT_EQ(plan.effects.size(), 7u);
    EXPECT_EQ(plan.effects[6].kind,
              UniqueItemMixSideEffectKind::RollRandomResultItem);

    RecordingSink sink;
    auto out = apply_unique_item_mix_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 7u);
    EXPECT_EQ(out.obtains, 0u);
    EXPECT_FALSE(out.obtain_flag_consumed);
    EXPECT_EQ(sink.obtain_count, 0u);
}

TEST(ApplyUniqueItemMixSideEffects, GateFailuresEmitSilentEmptyPlan) {
    struct Case {
        void (*mutate)(UniqueItemMixValidationInput&);
    };
    const Case cases[] = {
        {[](UniqueItemMixValidationInput& i) { i.basic_item_exists = false; }},
        {[](UniqueItemMixValidationInput& i) { i.all_materials_exist = false; }},
        {[](UniqueItemMixValidationInput& i) { i.mix_info_exists = false; }},
        {[](UniqueItemMixValidationInput& i) { i.enough_material_for_each_kind = false; }},
    };
    for (const auto& c : cases) {
        auto in = PassingGates();
        c.mutate(in);
        auto plan = unique_item_mix_side_effect_plan(
            in, 7, {}, 5, 33, 303, 999, 1);
        EXPECT_FALSE(plan.discard_materials);
        EXPECT_FALSE(plan.discard_basic);
        EXPECT_FALSE(plan.roll_result);
        EXPECT_FALSE(plan.obtain_result);
        EXPECT_TRUE(plan.effects.empty());

        RecordingSink sink;
        auto out = apply_unique_item_mix_side_effects(plan, sink);
        EXPECT_EQ(out.effects_applied, 0u);
        EXPECT_EQ(out.material_discards, 0u);
        EXPECT_EQ(out.basic_discards, 0u);
        EXPECT_EQ(out.rolls, 0u);
        EXPECT_EQ(out.obtains, 0u);
        EXPECT_TRUE(sink.calls.empty());
    }
}

TEST(ApplyUniqueItemMixSideEffects, EmptyPlanIsNoOp) {
    mxh::server::UniqueItemMixSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_unique_item_mix_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.material_discards, 0u);
    EXPECT_EQ(out.material_acks, 0u);
    EXPECT_EQ(out.material_logs, 0u);
    EXPECT_EQ(out.basic_discards, 0u);
    EXPECT_EQ(out.basic_acks, 0u);
    EXPECT_EQ(out.basic_logs, 0u);
    EXPECT_EQ(out.rolls, 0u);
    EXPECT_EQ(out.obtains, 0u);
    EXPECT_FALSE(out.materials_flag_consumed);
    EXPECT_FALSE(out.basic_flag_consumed);
    EXPECT_FALSE(out.roll_flag_consumed);
    EXPECT_FALSE(out.obtain_flag_consumed);
    EXPECT_FALSE(out.log_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}
