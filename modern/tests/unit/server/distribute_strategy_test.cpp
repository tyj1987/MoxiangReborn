#include <mxh/server/distribute_strategy.hpp>
#include <gtest/gtest.h>

using namespace mxh::server;

TEST(DistributeRandom, EmptyPartyHasNoReceiver) {
    EXPECT_FALSE(select_random_receiver({}, 7).has_value());
}

TEST(DistributeRandom, UsesModuloMemberCount) {
    std::vector<PartyReceiveMember> members{{10, 0, true}, {20, 0, true}, {30, 0, true}};
    EXPECT_EQ(select_random_receiver(members, 0), 0u);
    EXPECT_EQ(select_random_receiver(members, 4), 1u);
    EXPECT_EQ(select_random_receiver(members, 8), 2u);
}

TEST(DistributeRandom, SelectedUnavailableMemberAborts) {
    std::vector<PartyReceiveMember> members{{10, 0, true}, {20, 0, false}};
    EXPECT_FALSE(select_random_receiver(members, 1).has_value());
}

TEST(DistributeDamage, EmptyPartyHasNoReceiver) {
    EXPECT_FALSE(select_damage_receiver_legacy({}).has_value());
}

TEST(DistributeDamage, PreservesLegacyMissingBigDamageAssignmentBug) {
    std::vector<PartyReceiveMember> members{{10, 900, true}, {20, 1, true}, {30, 2, true}};
    EXPECT_EQ(select_damage_receiver_legacy(members), 2u);
}

TEST(DistributeDamage, LastPositiveDamageMemberWins) {
    std::vector<PartyReceiveMember> members{{10, 0, true}, {20, 100, true}, {30, 0, true}, {40, 1, true}};
    EXPECT_EQ(select_damage_receiver_legacy(members), 3u);
}

TEST(DistributeDamage, ZeroDamageTieUsesRandModuloTwo) {
    std::vector<PartyReceiveMember> members{{10, 0, true}, {20, 0, true}, {30, 0, true}};
    EXPECT_EQ(select_damage_receiver_legacy(members, {0, 1, 0}), 1u);
    EXPECT_EQ(select_damage_receiver_legacy(members, {1, 1, 1}), 2u);
}

TEST(DistributeDamage, SelectedUnavailableMemberAborts) {
    std::vector<PartyReceiveMember> members{{10, 5, true}, {20, 1, false}};
    EXPECT_FALSE(select_damage_receiver_legacy(members).has_value());
}

TEST(DistributeMoney, EmptyPartyProducesNoShare) {
    const auto share = split_party_money(100, 0);
    EXPECT_EQ(share.perMember, 0u);
    EXPECT_EQ(share.distributed, 0u);
    EXPECT_EQ(share.remainder, 0u);
}

TEST(DistributeMoney, UsesIntegerDivisionAndDropsRemainder) {
    const auto share = split_party_money(101, 3);
    EXPECT_EQ(share.perMember, 33u);
    EXPECT_EQ(share.distributed, 99u);
    EXPECT_EQ(share.remainder, 2u);
}

TEST(DistributeMoney, ExactDivisionHasNoRemainder) {
    const auto share = split_party_money(120, 4);
    EXPECT_EQ(share.perMember, 30u);
    EXPECT_EQ(share.distributed, 120u);
    EXPECT_EQ(share.remainder, 0u);
}

TEST(DistributeDropRatio, ZeroNeverDrops) {
    for (std::uint32_t randomValue = 0; randomValue < 100; ++randomValue)
        EXPECT_FALSE(legacy_drop_ratio_hit(0, randomValue));
}

TEST(DistributeDropRatio, HundredAlwaysDrops) {
    for (std::uint32_t randomValue = 0; randomValue < 100; ++randomValue)
        EXPECT_TRUE(legacy_drop_ratio_hit(100, randomValue));
}

TEST(DistributeDropRatio, FiftyMatchesLegacyModuloExpression) {
    EXPECT_TRUE(legacy_drop_ratio_hit(50, 0));
    EXPECT_FALSE(legacy_drop_ratio_hit(50, 1));
    EXPECT_TRUE(legacy_drop_ratio_hit(50, 2));
    EXPECT_FALSE(legacy_drop_ratio_hit(50, 99));
}

TEST(DistributeDropRatio, ThirtyThreeUsesIntegerDivisorThree) {
    EXPECT_TRUE(legacy_drop_ratio_hit(33, 0));
    EXPECT_FALSE(legacy_drop_ratio_hit(33, 1));
    EXPECT_TRUE(legacy_drop_ratio_hit(33, 3));
    EXPECT_TRUE(legacy_drop_ratio_hit(33, 99));
}

TEST(DistributeDropRatio, AboveHundredRejectedToAvoidLegacyDivideByZero) {
    EXPECT_FALSE(legacy_drop_ratio_hit(101, 0));
}

TEST(DistributeGate, RequiresMembersAndLevelCheck) {
    EXPECT_FALSE(can_distribute_item(0, true));
    EXPECT_FALSE(can_distribute_item(1, false));
    EXPECT_TRUE(can_distribute_item(1, true));
}
