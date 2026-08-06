#include <cstddef>

#include <mxh/server/agent_guild_notify_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

AgentGuildNotifyValidationInput join_ok() {
    AgentGuildNotifyValidationInput input{};
    input.action = AgentGuildNotifyAction::MunpaJoinSyn;
    input.user_found = true;
    input.filter_passed = true;
    return input;
}

AgentGuildNotifyValidationInput munha_ok() {
    AgentGuildNotifyValidationInput input{};
    input.action = AgentGuildNotifyAction::MunhaNameChangeOrOtherJoinSyn;
    input.user_found = true;
    return input;
}

AgentGuildNotifyValidationInput delete_user_ok() {
    AgentGuildNotifyValidationInput input{};
    input.action = AgentGuildNotifyAction::MunpaDeleteUserAlram;
    input.user_found = true;
    input.filter_passed = true;
    return input;
}

AgentGuildNotifyValidationInput create_ok() {
    AgentGuildNotifyValidationInput input{};
    input.action = AgentGuildNotifyAction::GuildCreateSyn;
    input.user_found = true;
    input.filter_passed = true;
    input.usable_name_passed = true;
    return input;
}

AgentGuildNotifyValidationInput nickname_ok() {
    AgentGuildNotifyValidationInput input{};
    input.action = AgentGuildNotifyAction::GuildGiveNicknameSyn;
    input.user_found = true;
    input.usable_name_passed = true;
    input.no_quote_chars = true;
    return input;
}

TEST(AgentGuildNotifyOutcome, MunpaJoinNotesWhenUserAndFilterPass) {
    EXPECT_EQ(classify_agent_guild_notify_outcome(join_ok()),
              AgentGuildNotifyOutcome::NotedUser);
}

TEST(AgentGuildNotifyOutcome, MunpaJoinNoUserPrecedesFilter) {
    auto input = join_ok();
    input.user_found = false;
    input.filter_passed = false;
    EXPECT_EQ(classify_agent_guild_notify_outcome(input),
              AgentGuildNotifyOutcome::NoUser);
}

TEST(AgentGuildNotifyOutcome, MunpaJoinFiltered) {
    auto input = join_ok();
    input.filter_passed = false;
    EXPECT_EQ(classify_agent_guild_notify_outcome(input),
              AgentGuildNotifyOutcome::Filtered);
}

TEST(AgentGuildNotifyOutcome, MunhaAlarmsWhenUserFound) {
    EXPECT_EQ(classify_agent_guild_notify_outcome(munha_ok()),
              AgentGuildNotifyOutcome::AlarmedMaster);
}

TEST(AgentGuildNotifyOutcome, MunhaNoUser) {
    auto input = munha_ok();
    input.user_found = false;
    EXPECT_EQ(classify_agent_guild_notify_outcome(input),
              AgentGuildNotifyOutcome::NoUser);
}

TEST(AgentGuildNotifyOutcome, MunpaDeleteNotesWhenUserAndFilterPass) {
    EXPECT_EQ(classify_agent_guild_notify_outcome(delete_user_ok()),
              AgentGuildNotifyOutcome::NotedUser);
}

TEST(AgentGuildNotifyOutcome, MunpaDeleteNoUser) {
    auto input = delete_user_ok();
    input.user_found = false;
    EXPECT_EQ(classify_agent_guild_notify_outcome(input),
              AgentGuildNotifyOutcome::NoUser);
}

TEST(AgentGuildNotifyOutcome, MunpaDeleteFiltered) {
    auto input = delete_user_ok();
    input.filter_passed = false;
    EXPECT_EQ(classify_agent_guild_notify_outcome(input),
              AgentGuildNotifyOutcome::Filtered);
}

TEST(AgentGuildNotifyOutcome, GuildCreateForwardsWhenAllGatesPass) {
    EXPECT_EQ(classify_agent_guild_notify_outcome(create_ok()),
              AgentGuildNotifyOutcome::ForwardedToMap);
}

TEST(AgentGuildNotifyOutcome, GuildCreateNoUser) {
    auto input = create_ok();
    input.user_found = false;
    EXPECT_EQ(classify_agent_guild_notify_outcome(input),
              AgentGuildNotifyOutcome::NoUser);
}

TEST(AgentGuildNotifyOutcome, GuildCreateInvalidCharacterIsFiltered) {
    auto input = create_ok();
    input.filter_passed = false;
    EXPECT_EQ(classify_agent_guild_notify_outcome(input),
              AgentGuildNotifyOutcome::Filtered);
}

TEST(AgentGuildNotifyOutcome, GuildCreateUnusableNameNacks) {
    auto input = create_ok();
    input.usable_name_passed = false;
    EXPECT_EQ(classify_agent_guild_notify_outcome(input),
              AgentGuildNotifyOutcome::CreateNackName);
}

TEST(AgentGuildNotifyOutcome, GuildNicknameForwardsWhenAllGatesPass) {
    EXPECT_EQ(classify_agent_guild_notify_outcome(nickname_ok()),
              AgentGuildNotifyOutcome::ForwardedToMap);
}

TEST(AgentGuildNotifyOutcome, GuildNicknameNoUser) {
    auto input = nickname_ok();
    input.user_found = false;
    EXPECT_EQ(classify_agent_guild_notify_outcome(input),
              AgentGuildNotifyOutcome::NoUser);
}

TEST(AgentGuildNotifyOutcome, GuildNicknameUnusableNameNacks) {
    auto input = nickname_ok();
    input.usable_name_passed = false;
    EXPECT_EQ(classify_agent_guild_notify_outcome(input),
              AgentGuildNotifyOutcome::NickNackFilter);
}

TEST(AgentGuildNotifyOutcome, GuildNicknameQuoteCharacterNacks) {
    auto input = nickname_ok();
    input.no_quote_chars = false;
    EXPECT_EQ(classify_agent_guild_notify_outcome(input),
              AgentGuildNotifyOutcome::NickNackFilter);
}

TEST(AgentGuildNotifyOutcome, LegacyOutcomeValueOverlapIsPreserved) {
    EXPECT_EQ(static_cast<std::uint8_t>(AgentGuildNotifyOutcome::AlarmedMaster), 2u);
    EXPECT_EQ(static_cast<std::uint8_t>(AgentGuildNotifyOutcome::CreateNackName), 2u);
}

TEST(AgentGuildNotifyPlan, MunpaJoinIncludesMasterAlarmWhenMasterOnline) {
    auto input = join_ok();
    input.master_found = true;
    const auto plan = agent_guild_notify_side_effect_plan(input, 100u, 200u);
    ASSERT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 4u);
    EXPECT_EQ(plan.effects[0].kind, AgentGuildNotifySideEffectKind::CopyNoteBuffers);
    EXPECT_EQ(plan.effects[1].kind, AgentGuildNotifySideEffectKind::FilterCheckGuildName);
    EXPECT_EQ(plan.effects[2].kind, AgentGuildNotifySideEffectKind::NoteServerSendtoPlayer);
    EXPECT_EQ(plan.effects[3].kind, AgentGuildNotifySideEffectKind::SendJoinMasterAlram);
    EXPECT_EQ(plan.effects[3].object_id, 100u);
    EXPECT_EQ(plan.effects[3].master_id, 200u);
}

TEST(AgentGuildNotifyPlan, MunpaJoinOmitsMasterAlarmWhenMasterOffline) {
    const auto plan = agent_guild_notify_side_effect_plan(join_ok(), 100u, 200u);
    ASSERT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 3u);
    EXPECT_EQ(plan.effects.back().kind,
              AgentGuildNotifySideEffectKind::NoteServerSendtoPlayer);
}

TEST(AgentGuildNotifyPlan, MunpaJoinRejectedHasNoEffects) {
    auto input = join_ok();
    input.filter_passed = false;
    const auto plan = agent_guild_notify_side_effect_plan(input, 100u, 200u);
    EXPECT_FALSE(plan.dispatched);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.effects.empty());
}

TEST(AgentGuildNotifyPlan, MunhaEmitsMasterAlarm) {
    const auto plan = agent_guild_notify_side_effect_plan(munha_ok(), 100u, 0u);
    ASSERT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              AgentGuildNotifySideEffectKind::SendMunhaMasterAlram);
    EXPECT_EQ(plan.effects[0].object_id, 100u);
}

TEST(AgentGuildNotifyPlan, MunpaDeleteEmitsNoteSequence) {
    const auto plan =
        agent_guild_notify_side_effect_plan(delete_user_ok(), 100u, 0u);
    ASSERT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 3u);
    EXPECT_EQ(plan.effects[0].kind, AgentGuildNotifySideEffectKind::CopyNoteBuffers);
    EXPECT_EQ(plan.effects[1].kind, AgentGuildNotifySideEffectKind::FilterCheckGuildName);
    EXPECT_EQ(plan.effects[2].kind, AgentGuildNotifySideEffectKind::NoteServerSendtoPlayer);
}

TEST(AgentGuildNotifyPlan, GuildCreateForwardsToMap) {
    const auto plan = agent_guild_notify_side_effect_plan(create_ok(), 300u, 0u);
    ASSERT_TRUE(plan.dispatched);
    ASSERT_TRUE(plan.forward_to_map);
    ASSERT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              AgentGuildNotifySideEffectKind::ForwardToMapServer);
    EXPECT_EQ(plan.effects[0].object_id, 300u);
}

TEST(AgentGuildNotifyPlan, GuildCreateInvalidNameSendsLegacyNack) {
    auto input = create_ok();
    input.usable_name_passed = false;
    const auto plan = agent_guild_notify_side_effect_plan(input, 300u, 0u);
    EXPECT_FALSE(plan.dispatched);
    EXPECT_FALSE(plan.forward_to_map);
    ASSERT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentGuildNotifySideEffectKind::SendCreateNack);
    EXPECT_EQ(plan.effects[0].nack_code, LEGACY_EGUILDERR_CREATE_NAME);
}

TEST(AgentGuildNotifyPlan, GuildNicknameForwardsToMap) {
    const auto plan =
        agent_guild_notify_side_effect_plan(nickname_ok(), 400u, 0u);
    ASSERT_TRUE(plan.dispatched);
    ASSERT_TRUE(plan.forward_to_map);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              AgentGuildNotifySideEffectKind::ForwardToMapServer);
}

TEST(AgentGuildNotifyPlan, GuildNicknameInvalidNameSendsLegacyNack) {
    auto input = nickname_ok();
    input.no_quote_chars = false;
    const auto plan =
        agent_guild_notify_side_effect_plan(input, 400u, 0u);
    EXPECT_FALSE(plan.dispatched);
    EXPECT_FALSE(plan.forward_to_map);
    ASSERT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentGuildNotifySideEffectKind::SendNickNack);
    EXPECT_EQ(plan.effects[0].nack_code, LEGACY_EGUILDERR_NICK_FILTER);
}

TEST(AgentGuildNotifyPlan, MissingUserHasNoEffects) {
    auto input = create_ok();
    input.user_found = false;
    const auto plan = agent_guild_notify_side_effect_plan(input, 500u, 0u);
    EXPECT_FALSE(plan.dispatched);
    EXPECT_FALSE(plan.forward_to_map);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.effects.empty());
}

TEST(AgentGuildNotifyPlan, PlanIsIdempotent) {
    auto input = join_ok();
    input.master_found = true;
    const auto first = agent_guild_notify_side_effect_plan(input, 100u, 200u);
    const auto second = agent_guild_notify_side_effect_plan(input, 100u, 200u);
    EXPECT_EQ(first.dispatched, second.dispatched);
    EXPECT_EQ(first.forward_to_map, second.forward_to_map);
    EXPECT_EQ(first.send_nack, second.send_nack);
    ASSERT_EQ(first.effects.size(), second.effects.size());
    for (std::size_t index = 0; index < first.effects.size(); ++index) {
        EXPECT_EQ(first.effects[index].kind, second.effects[index].kind);
        EXPECT_EQ(first.effects[index].object_id, second.effects[index].object_id);
        EXPECT_EQ(first.effects[index].master_id, second.effects[index].master_id);
        EXPECT_EQ(first.effects[index].nack_code, second.effects[index].nack_code);
    }
}
