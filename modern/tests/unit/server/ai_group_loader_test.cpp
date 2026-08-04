#include "mxh/server/ai_group_loader.hpp"
#include "mxh/server/ai_system.hpp"

#include <gtest/gtest.h>

namespace {
using mxh::server::AISystem;
using mxh::server::AiRegenDelayKind;
using mxh::server::load_ai_group_list_bin;
using mxh::server::parse_ai_group_list;
}

TEST(AiGroupLoader, ParsesLegacyGroupFields) {
    const auto list = parse_ai_group_list(
        "$Group 1\n{\n#MAXOBJECT 2\n#GROUPNAME wolves\n"
        "#PROPERTY 7\n#ADDCONDITION 1 0.5 25000 1\n"
        "#ADD 32 100001 105 44653 7829 1\n}");
    ASSERT_TRUE(list.has_value());
    ASSERT_EQ(list->groups.size(), 1u);
    const auto& group = list->groups[0];
    EXPECT_EQ(group.group_id, 1u);
    EXPECT_FALSE(group.unique);
    EXPECT_EQ(group.max_object, 2u);
    EXPECT_EQ(group.property, 7u);
    EXPECT_EQ(group.group_name, "wolves");
    ASSERT_EQ(group.conditions.size(), 1u);
    EXPECT_EQ(group.conditions[0].target_group_id, 1u);
    EXPECT_FLOAT_EQ(group.conditions[0].remainder_ratio, 0.5f);
    EXPECT_EQ(group.conditions[0].regen_delay, 25000u);
    EXPECT_TRUE(group.conditions[0].regen);
    ASSERT_EQ(group.spawns.size(), 1u);
    EXPECT_EQ(group.spawns[0].object_kind, 32u);
    EXPECT_EQ(group.spawns[0].source_object_id, 100001u);
    EXPECT_EQ(group.spawns[0].monster_kind, 105u);
    EXPECT_FLOAT_EQ(group.spawns[0].pos_x, 44653.0f);
    EXPECT_FLOAT_EQ(group.spawns[0].pos_z, 7829.0f);
    EXPECT_TRUE(group.spawns[0].initially_dead);
}

TEST(AiGroupLoader, ParsesMultipleGroupsAndLookup) {
    const auto list = parse_ai_group_list(
        "$Group 1\n{\n#ADD 1 10 20 1 2 0\n}\n"
        "$GROUP 2\n{\n#ADD 2 11 21 3 4 1\n}");
    ASSERT_TRUE(list.has_value());
    EXPECT_EQ(list->groups.size(), 2u);
    EXPECT_EQ(list->spawn_count(), 2u);
    ASSERT_NE(list->find_group(2u), nullptr);
    EXPECT_EQ(list->find_group(2u)->spawns[0].source_object_id, 11u);
    EXPECT_EQ(list->find_group(99u), nullptr);
}

TEST(AiGroupLoader, ParsesUniqueGroupCommands) {
    const auto list = parse_ai_group_list(
        "$Unique 9\n{\n#UNIQUEADD 32 7 44 1.5 2.5 0\n"
        "#UNIQUEADDCONDITION 3 0.25 5000 0\n}");
    ASSERT_TRUE(list.has_value());
    ASSERT_EQ(list->groups.size(), 1u);
    EXPECT_TRUE(list->groups[0].unique);
    EXPECT_EQ(list->groups[0].spawns[0].monster_kind, 44u);
    EXPECT_FALSE(list->groups[0].conditions[0].regen);
}

TEST(AiGroupLoader, ParsesFieldBossPositions) {
    const auto list = parse_ai_group_list(
        "#FILEDBOSSREGENPOSITION 10.5 20.25\n"
        "$Group 1\n{\n#MAXOBJECT 0\n}");
    ASSERT_TRUE(list.has_value());
    ASSERT_EQ(list->field_boss_positions.size(), 1u);
    EXPECT_FLOAT_EQ(list->field_boss_positions[0].x, 10.5f);
    EXPECT_FLOAT_EQ(list->field_boss_positions[0].z, 20.25f);
}

TEST(AiGroupLoader, ParsesFixedAndRandomDelayKinds) {
    const auto fixed = parse_ai_group_list(
        "$Unique 1\n{\n#UNIQUEREGENDELAY 5\n}");
    ASSERT_TRUE(fixed.has_value());
    EXPECT_EQ(fixed->groups[0].regen_delay.kind,
              AiRegenDelayKind::FixedUnique);
    EXPECT_EQ(fixed->groups[0].regen_delay.min_minutes, 5u);
    EXPECT_EQ(fixed->groups[0].regen_delay.max_minutes, 5u);

    const auto random = parse_ai_group_list(
        "$Group 2\n{\n#RANDOMREGENDELAY 3 8\n}");
    ASSERT_TRUE(random.has_value());
    EXPECT_EQ(random->groups[0].regen_delay.kind,
              AiRegenDelayKind::RandomGroup);
    EXPECT_EQ(random->groups[0].regen_delay.min_minutes, 3u);
    EXPECT_EQ(random->groups[0].regen_delay.max_minutes, 8u);
}

TEST(AiGroupLoader, DistinguishesUniqueRandomDelayVariants) {
    const auto legacy = parse_ai_group_list(
        "$Unique 1\n{\n#UNIQUERANDOMREGENDELAY 2 9\n}");
    const auto ranged = parse_ai_group_list(
        "$Unique 1\n{\n#UNIQUERANDOMREGENDELAY2 2 9\n}");
    ASSERT_TRUE(legacy.has_value());
    ASSERT_TRUE(ranged.has_value());
    EXPECT_EQ(legacy->groups[0].regen_delay.kind,
              AiRegenDelayKind::RandomUnique);
    EXPECT_EQ(ranged->groups[0].regen_delay.kind,
              AiRegenDelayKind::RandomUniqueRange);
}

TEST(AiGroupLoader, AcceptsEmptyGroupNameAndCrlf) {
    const auto list = parse_ai_group_list(
        "$Group\t1\r\n{\r\n#GROUPNAME\r\n#PROPERTY\t0\r\n}");
    ASSERT_TRUE(list.has_value());
    EXPECT_TRUE(list->groups[0].group_name.empty());
}

TEST(AiGroupLoader, RejectsDirectiveOutsideGroup) {
    EXPECT_FALSE(parse_ai_group_list("#ADD 1 2 3 4 5 0").has_value());
}

TEST(AiGroupLoader, RejectsMalformedAndOverflowFields) {
    EXPECT_FALSE(parse_ai_group_list(
        "$Group nope\n{\n}").has_value());
    EXPECT_FALSE(parse_ai_group_list(
        "$Group 1\n{\n#ADD 256 2 3 4 5 0\n}").has_value());
    EXPECT_FALSE(parse_ai_group_list(
        "$Group 1\n{\n#PROPERTY 65536\n}").has_value());
}

TEST(AiGroupLoader, RejectsUnknownCommands) {
    EXPECT_FALSE(parse_ai_group_list(
        "$Group 1\n{\n#BOGUS 1\n}").has_value());
}

TEST(AiGroupLoader, LoadsRealMonster10Bin) {
    const auto list = load_ai_group_list_bin(
        "../../../deploy/server/Distribute/Resource/Server/Monster_10.bin");
    ASSERT_TRUE(list.has_value());
    EXPECT_EQ(list->groups.size(), 114u);
    EXPECT_EQ(list->spawn_count(), 228u);
    EXPECT_EQ(list->field_boss_positions.size(), 3u);
    const auto* group = list->find_group(1u);
    ASSERT_NE(group, nullptr);
    EXPECT_EQ(group->max_object, 2u);
    ASSERT_EQ(group->spawns.size(), 2u);
    EXPECT_EQ(group->spawns[0].source_object_id, 100001u);
    EXPECT_EQ(group->spawns[0].monster_kind, 105u);
}

TEST(AiGroupLoader, LoadsRealEmptyMonster12Bin) {
    const auto list = load_ai_group_list_bin(
        "../../../deploy/server/Distribute/Resource/Server/Monster_12.bin");
    ASSERT_TRUE(list.has_value());
    EXPECT_TRUE(list->groups.empty());
    EXPECT_EQ(list->spawn_count(), 0u);
}

TEST(AiGroupLoader, MissingBinReturnsNullopt) {
    EXPECT_FALSE(load_ai_group_list_bin("missing-monster.bin").has_value());
}

TEST(AISystemGroupLoader, ReplacesStateTransactionally) {
    AISystem system;
    ASSERT_TRUE(system.load_ai_group_list(
        "../../../deploy/server/Distribute/Resource/Server/Monster_10.bin"));
    EXPECT_EQ(system.group_list().groups.size(), 114u);
    EXPECT_EQ(system.group_list().spawn_count(), 228u);
    EXPECT_FALSE(system.load_ai_group_list("missing-monster.bin"));
    EXPECT_EQ(system.group_list().groups.size(), 114u);
}
