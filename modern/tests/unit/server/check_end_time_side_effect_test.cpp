// D4.35 CheckEndTime side-effect dispatcher tests.

#include <mxh/server/check_end_time_side_effect.hpp>

#include <gtest/gtest.h>

#include <cstdint>

using namespace mxh::server;
using namespace mxh::game;

namespace {

ShopItemWithTime make_row(std::uint16_t pos, std::uint16_t icon,
                          std::uint32_t db_idx) {
    ShopItemWithTime row{};
    row.ShopItem.ItemBase.Position = pos;
    row.ShopItem.ItemBase.wIconIdx = icon;
    row.ShopItem.ItemBase.dwDBIdx = db_idx;
    return row;
}

}  // namespace

TEST(CheckEndTimeSideEffect, FiveStepsInLegacyOrder) {
    auto row = make_row(/*pos=*/5, /*icon=*/10, /*db_idx=*/42);
    auto steps = check_end_time_side_effect(
        row, /*player_id=*/1, ShopItemDupSlot::None);
    ASSERT_EQ(steps.size(), 4u);  // no dup bump when slot == None
    EXPECT_EQ(steps[0].kind, CheckEndTimeStepKind::DiscardItemAttempt);
    EXPECT_EQ(steps[0].item_pos, 5u);
    EXPECT_EQ(steps[0].w_icon_idx, 10u);
    EXPECT_EQ(steps[0].db_idx, 42u);
    EXPECT_EQ(steps[1].kind, CheckEndTimeStepKind::BroadcastUseEnd);
    EXPECT_EQ(steps[1].w_icon_idx, 10u);
    EXPECT_EQ(steps[1].db_idx, 42u);
    EXPECT_EQ(steps[2].kind, CheckEndTimeStepKind::ShopItemDeleteToDB);
    EXPECT_EQ(steps[2].db_idx, 42u);
    EXPECT_EQ(steps[3].kind, CheckEndTimeStepKind::LogItemMoney);
    EXPECT_EQ(steps[3].w_icon_idx, 10u);
}

TEST(CheckEndTimeSideEffect, DupBumpStepInsertedAfterDiscard) {
    auto row = make_row(/*pos=*/3, /*icon=*/20, /*db_idx=*/99);
    auto steps = check_end_time_side_effect(
        row, /*player_id=*/1, ShopItemDupSlot::Charm);
    ASSERT_EQ(steps.size(), 5u);
    EXPECT_EQ(steps[0].kind, CheckEndTimeStepKind::DiscardItemAttempt);
    EXPECT_EQ(steps[1].kind, CheckEndTimeStepKind::BumpDupCounter);
    EXPECT_EQ(steps[1].dup_slot, ShopItemDupSlot::Charm);
    EXPECT_EQ(steps[2].kind, CheckEndTimeStepKind::BroadcastUseEnd);
    EXPECT_EQ(steps[3].kind, CheckEndTimeStepKind::ShopItemDeleteToDB);
    EXPECT_EQ(steps[4].kind, CheckEndTimeStepKind::LogItemMoney);
}

TEST(CheckEndTimeSideEffect, DbIdxFlowsThroughEveryStep) {
    auto row = make_row(/*pos=*/7, /*icon=*/15, /*db_idx=*/12345);
    auto steps = check_end_time_side_effect(
        row, /*player_id=*/2, ShopItemDupSlot::Incantation);
    for (const auto& s : steps) {
        EXPECT_EQ(s.db_idx, 12345u);
    }
}

TEST(CheckEndTimeSideEffect, AllDupSlotsAreAccepted) {
    auto row = make_row(/*pos=*/0, /*icon=*/1, /*db_idx=*/0);
    for (auto slot : {ShopItemDupSlot::Incantation, ShopItemDupSlot::Charm,
                      ShopItemDupSlot::Herb, ShopItemDupSlot::Sundries,
                      ShopItemDupSlot::PetEquip}) {
        auto steps = check_end_time_side_effect(row, /*player_id=*/1, slot);
        ASSERT_EQ(steps.size(), 5u);
        EXPECT_EQ(steps[1].dup_slot, slot);
    }
}
