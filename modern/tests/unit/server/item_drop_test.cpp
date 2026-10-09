#include <mxh/server/item_drop.hpp>
#include <gtest/gtest.h>

using namespace mxh::server;

TEST(ItemDropConstants, MatchLegacy) {
    EXPECT_EQ(DROP_ITEM_KIND_COUNT, 5u);
    EXPECT_EQ(MAX_DROPITEM_NUM, 20u);
    EXPECT_EQ(MAX_DROP_ITEM_PERCENT, 10000u);
    EXPECT_EQ(MAX_DROP_MAPITEM_PERCENT, 1000000u);
}

TEST(ItemDropKinds, MatchLegacyOrdinals) {
    EXPECT_EQ(static_cast<int>(DropItemKind::NoItem), 0);
    EXPECT_EQ(static_cast<int>(DropItemKind::Money), 1);
    EXPECT_EQ(static_cast<int>(DropItemKind::Item1), 2);
    EXPECT_EQ(static_cast<int>(DropItemKind::Item3), 4);
}

TEST(ItemDropRates, AppliesMoneyAndItemEventRatesSeparately) {
    DropRateContext context{{100, 200, 300, 400, 500}, 2.0f, 3.0f};
    const auto table = calculate_drop_rates(context);
    EXPECT_EQ(table.rates, (std::array<std::uint32_t, 5>{100, 400, 900, 1200, 1500}));
}

TEST(ItemDropRates, AppliesPartyRateOnlyToItems) {
    DropRateContext context{{100, 200, 100, 100, 100}, 1.0f, 1.0f, 1.5f};
    const auto table = calculate_drop_rates(context);
    EXPECT_EQ(table.rates[1], 200u);
    EXPECT_EQ(table.rates[2], 150u);
}

TEST(ItemDropRates, ZeroPartyRatePreservesLegacyNoMultiplierBranch) {
    DropRateContext context{{0, 0, 100, 100, 100}, 1.0f, 1.0f, 0.0f};
    EXPECT_EQ(calculate_drop_rates(context).rates[2], 100u);
}

TEST(ItemDropRates, TalismanUsesIntegerMultiplierWithPointZeroZeroOneFix) {
    DropRateContext context{{0, 0, 100, 100, 100}};
    context.addItemDropPercent = 100;
    EXPECT_EQ(calculate_drop_rates(context).rates[2], 200u);
    context.addItemDropPercent = 99;
    EXPECT_EQ(calculate_drop_rates(context).rates[2], 100u);
    context.addItemDropPercent = 199;
    EXPECT_EQ(calculate_drop_rates(context).rates[2], 200u);
}

TEST(ItemDropRates, ChannelRateAppliesAfterTalisman) {
    DropRateContext context{{0, 0, 100, 0, 0}};
    context.addItemDropPercent = 100;
    context.channelDropRate = 1.5f;
    EXPECT_EQ(calculate_drop_rates(context).rates[2], 300u);
}

TEST(ItemDropRates, RebalancesNoItemWhenTotalExceedsTenThousand) {
    DropRateContext context{{9000, 100, 100, 100, 100}};
    context.channelDropRate = 3.0f;
    const auto table = calculate_drop_rates(context);
    EXPECT_EQ(table.totalRate, 10000u);
    EXPECT_EQ(table.rates[0], 9000u);
}

TEST(ItemDropRates, ClearsNoItemWhenRewardsAloneExceedTenThousand) {
    DropRateContext context{{100, 3000, 3000, 3000, 3000}};
    const auto table = calculate_drop_rates(context);
    EXPECT_EQ(table.rates[0], 0u);
    EXPECT_EQ(table.totalRate, 12000u);
}

TEST(ItemDropSelection, RejectsInvalidRollsAndZeroTotal) {
    EXPECT_FALSE(select_drop_kind({}, 1).has_value());
    DropRateTable table{{1, 1, 1, 1, 1}, 5};
    EXPECT_FALSE(select_drop_kind(table, 0).has_value());
    EXPECT_FALSE(select_drop_kind(table, 6).has_value());
}

TEST(ItemDropSelection, UsesOneBasedInclusiveIntervals) {
    DropRateTable table{{2, 3, 4, 0, 1}, 10};
    EXPECT_EQ(select_drop_kind(table, 1), DropItemKind::NoItem);
    EXPECT_EQ(select_drop_kind(table, 2), DropItemKind::NoItem);
    EXPECT_EQ(select_drop_kind(table, 3), DropItemKind::Money);
    EXPECT_EQ(select_drop_kind(table, 5), DropItemKind::Money);
    EXPECT_EQ(select_drop_kind(table, 6), DropItemKind::Item1);
    EXPECT_EQ(select_drop_kind(table, 10), DropItemKind::Item3);
}

TEST(ItemDropMoney, UsesMinimumWhenRangeIsDegenerate) {
    EXPECT_EQ(monster_drop_money(50, 50, 999, 1.0f), 50u);
    EXPECT_EQ(monster_drop_money(60, 50, 999, 2.0f), 120u);
}

TEST(ItemDropMoney, AppliesGetMoneyEventRateAfterRandomSelection) {
    EXPECT_EQ(monster_drop_money(10, 100, 73, 1.5f), 109u);
}

TEST(MonsterDropPool, ReloadCopiesConfiguredRates) {
    MonsterDropPool pool{{{10, 2, 0}, {20, 3, 0}}, 0};
    EXPECT_TRUE(reload_monster_drop_pool(pool));
    EXPECT_EQ(pool.currentTotalRate, 5u);
    EXPECT_EQ(pool.entries[0].currentRate, 2u);
}

TEST(MonsterDropPool, AllZeroCannotReload) {
    MonsterDropPool pool{{{10, 0, 0}}, 0};
    EXPECT_FALSE(reload_monster_drop_pool(pool));
}

TEST(MonsterDropPool, DrawUsesOneBasedIntervalsAndDepletesWeight) {
    MonsterDropPool pool{{{10, 2, 2}, {20, 3, 3}}, 5};
    EXPECT_EQ(draw_monster_drop_item(pool, 2), 10u);
    EXPECT_EQ(pool.entries[0].currentRate, 1u);
    EXPECT_EQ(pool.currentTotalRate, 4u);
    EXPECT_EQ(draw_monster_drop_item(pool, 2), 20u);
}

TEST(MonsterDropPool, ZeroItemIdConsumesWeightButDropsNothing) {
    MonsterDropPool pool{{{0, 1, 1}}, 1};
    EXPECT_FALSE(draw_monster_drop_item(pool, 1).has_value());
    EXPECT_EQ(pool.currentTotalRate, 0u);
}

TEST(MonsterDropPool, EmptyCurrentPoolAutomaticallyReloads) {
    MonsterDropPool pool{{{99, 1, 0}}, 0};
    EXPECT_EQ(draw_monster_drop_item(pool, 1), 99u);
}

TEST(MonsterDropChoices, CanonicalKind105FiveCategoryBoundaries) {
    mxh::compat::MonsterVisual monster;
    monster.drop_rates = {5400u, 660u, 679u, 1u, 3260u};
    monster.item_list_ids = {313u, 314u, 315u};
    monster.max_drop_item_num = 1u;
    const auto profile = monster_drop_profile_from(monster);
    EXPECT_EQ(profile.rates,
              (std::array<std::uint32_t, 5>{3260u, 5400u, 660u, 679u, 1u}));
    EXPECT_TRUE(valid_monster_drop_profile(profile));
    const auto choose = [&](std::uint32_t roll) {
        const std::array<std::uint32_t, 1> rolls{roll};
        return select_monster_drop_choices(profile, {}, rolls);
    };
    const auto no_item = choose(3260u);
    ASSERT_TRUE(no_item);
    EXPECT_EQ((*no_item)[0].kind, DropItemKind::NoItem);
    EXPECT_EQ((*no_item)[0].item_list_id, 0u);
    const auto money = choose(3261u);
    ASSERT_TRUE(money);
    EXPECT_EQ((*money)[0].kind, DropItemKind::Money);
    EXPECT_EQ((*money)[0].item_list_id, 0u);
    EXPECT_EQ((*choose(8660u))[0].kind, DropItemKind::Money);
    EXPECT_EQ((*choose(8661u))[0].item_list_id, 313u);
    EXPECT_EQ((*choose(9320u))[0].item_list_id, 313u);
    EXPECT_EQ((*choose(9321u))[0].item_list_id, 314u);
    EXPECT_EQ((*choose(9999u))[0].item_list_id, 314u);
    EXPECT_EQ((*choose(10000u))[0].item_list_id, 315u);
    EXPECT_FALSE(choose(0u));
    EXPECT_FALSE(choose(10001u));
}

TEST(MonsterDropChoices, MaxRollCountAndMissingListFailClosed) {
    MonsterDropProfile profile{{0u, 0u, 10000u, 0u, 0u},
                               {313u, 314u, 315u}, 0u};
    EXPECT_TRUE(select_monster_drop_choices(profile, {}, {}).has_value());
    EXPECT_TRUE(select_monster_drop_choices(profile, {}, {})->empty());
    profile.max_rolls = 2u;
    const std::array<std::uint32_t, 1> short_rolls{1u};
    EXPECT_FALSE(select_monster_drop_choices(profile, {}, short_rolls));
    const std::array<std::uint32_t, 3> rolls{1u, 10000u, 0u};
    const auto choices = select_monster_drop_choices(profile, {}, rolls);
    ASSERT_TRUE(choices);
    ASSERT_EQ(choices->size(), 2u);
    EXPECT_EQ((*choices)[0].item_list_id, 313u);
    EXPECT_EQ((*choices)[1].item_list_id, 313u);
    profile.item_list_ids[0] = 0u;
    EXPECT_FALSE(select_monster_drop_choices(profile, {}, rolls));
    profile.item_list_ids[0] = 313u;
    profile.rates[0] = 1u;
    EXPECT_FALSE(valid_monster_drop_profile(profile));
    EXPECT_FALSE(select_monster_drop_choices(profile, {}, rolls));
}

TEST(MapItemDrop, SelectUsesOneBasedInclusiveIntervals) {
    MapDropTable table{1, 1, {{10, 2, 0, 1}, {20, 3, 0, 1}}, 5};
    EXPECT_EQ(select_map_drop_item(table, 2), 0u);
    EXPECT_EQ(select_map_drop_item(table, 3), 1u);
}

TEST(MapItemDrop, NoItemEntryNeverDrops) {
    MapDropTable table{1, 1, {{0, 1, 0, 10}}, 1};
    EXPECT_FALSE(try_map_drop(table, 1).has_value());
}

TEST(MapItemDrop, EnforcesMaximumDropCount) {
    MapDropTable table{1, 1, {{77, 1, 0, 1}}, 1};
    EXPECT_EQ(try_map_drop(table, 1), 77u);
    EXPECT_FALSE(try_map_drop(table, 1).has_value());
    EXPECT_EQ(table.items[0].dropCount, 1u);
}

TEST(MapItemDrop, ResetClearsAllCounts) {
    MapDropTable table{1, 1, {{10, 1, 2, 3}, {20, 1, 4, 5}}, 2};
    reset_map_drop_counts(table);
    EXPECT_EQ(table.items[0].dropCount, 0u);
    EXPECT_EQ(table.items[1].dropCount, 0u);
}

TEST(MapItemDrop, ResetWindowMatchesLegacyZeroThroughTenMinutes) {
    EXPECT_TRUE(is_map_drop_reset_window(5, 0, 0, 5, 0));
    EXPECT_TRUE(is_map_drop_reset_window(5, 0, 10, 5, 0));
    EXPECT_FALSE(is_map_drop_reset_window(5, 0, 11, 5, 0));
    EXPECT_FALSE(is_map_drop_reset_window(4, 0, 0, 5, 0));
}
