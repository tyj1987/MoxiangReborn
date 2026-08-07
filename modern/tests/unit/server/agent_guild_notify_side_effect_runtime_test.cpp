// agent_guild_notify_side_effect_runtime_test.cpp
//
// Verifies apply_agent_guild_notify_side_effects() (the runtime
// orchestrator for the legacy agent guild-notify side-effect chains)
// walks the data-plane plan and dispatches each entry: the note chains
// / master alarms / forward / NACK branches in legacy order.

#include <mxh/server/agent_guild_notify_side_effect.hpp>
#include <mxh/server/agent_guild_notify_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::AgentGuildNotifyAction;
using mxh::server::AgentGuildNotifySideEffectSink;
using mxh::server::AgentGuildNotifyValidationInput;
using mxh::server::LEGACY_EGUILDERR_CREATE_NAME;
using mxh::server::LEGACY_EGUILDERR_NICK_FILTER;
using mxh::server::apply_agent_guild_notify_side_effects;
using mxh::server::agent_guild_notify_side_effect_plan;

class RecordingSink final : public AgentGuildNotifySideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_object_id = 0;
    std::uint32_t last_master_id = 0;
    std::uint8_t last_nack_code = 0;

    void copy_note_buffers(std::uint32_t object_id) override {
        calls.push_back("copy");
        last_object_id = object_id;
    }
    void filter_check_guild_name(std::uint32_t object_id) override {
        calls.push_back("filter");
        last_object_id = object_id;
    }
    void note_server_sendto_player(std::uint32_t object_id) override {
        calls.push_back("send");
        last_object_id = object_id;
    }
    void send_join_master_alram(std::uint32_t object_id,
                                std::uint32_t master_id) override {
        calls.push_back("joinmaster");
        last_object_id = object_id;
        last_master_id = master_id;
    }
    void send_munha_master_alram(std::uint32_t object_id) override {
        calls.push_back("munhamaster");
        last_object_id = object_id;
    }
    void send_create_nack(std::uint32_t object_id,
                          std::uint8_t nack_code) override {
        calls.push_back("createnack");
        last_object_id = object_id;
        last_nack_code = nack_code;
    }
    void send_nick_nack(std::uint32_t object_id,
                        std::uint8_t nack_code) override {
        calls.push_back("nicknack");
        last_object_id = object_id;
        last_nack_code = nack_code;
    }
    void forward_to_map_server(std::uint32_t object_id) override {
        calls.push_back("forward");
        last_object_id = object_id;
    }
};

}  // namespace

TEST(ApplyAgentGuildNotifySideEffects, MunpaJoinEmitsChainPlusMasterAlarm) {
    AgentGuildNotifyValidationInput in;
    in.action = AgentGuildNotifyAction::MunpaJoinSyn;
    in.user_found = true;
    in.filter_passed = true;
    in.master_found = true;
    auto plan = agent_guild_notify_side_effect_plan(
        in, /*object_id=*/0x00260027u, /*master_id=*/88);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 4u);

    RecordingSink sink;
    auto out = apply_agent_guild_notify_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 4u);
    EXPECT_EQ(out.copies, 1u);
    EXPECT_EQ(out.filters, 1u);
    EXPECT_EQ(out.note_sends, 1u);
    EXPECT_EQ(out.join_alarms, 1u);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>(
                  {"copy", "filter", "send", "joinmaster"}));
    EXPECT_EQ(sink.last_master_id, 88u);
}

TEST(ApplyAgentGuildNotifySideEffects, MunpaJoinOmitsMasterAlarmWhenOffline) {
    AgentGuildNotifyValidationInput in;
    in.action = AgentGuildNotifyAction::MunpaJoinSyn;
    in.user_found = true;
    in.filter_passed = true;
    in.master_found = false;
    auto plan = agent_guild_notify_side_effect_plan(in, 7, 0);
    ASSERT_EQ(plan.effects.size(), 3u);

    RecordingSink sink;
    auto out = apply_agent_guild_notify_side_effects(plan, sink);
    EXPECT_EQ(out.join_alarms, 0u);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({"copy", "filter", "send"}));
}

TEST(ApplyAgentGuildNotifySideEffects, MunhaAndMunpaDeleteChains) {
    {
        AgentGuildNotifyValidationInput in;
        in.action = AgentGuildNotifyAction::MunhaNameChangeOrOtherJoinSyn;
        in.user_found = true;
        auto plan = agent_guild_notify_side_effect_plan(in, 1, 0);
        ASSERT_EQ(plan.effects.size(), 1u);
        RecordingSink sink;
        (void)apply_agent_guild_notify_side_effects(plan, sink);
        EXPECT_EQ(sink.calls,
                  std::vector<std::string>({"munhamaster"}));
    }
    {
        AgentGuildNotifyValidationInput in;
        in.action = AgentGuildNotifyAction::MunpaDeleteUserAlram;
        in.user_found = true;
        in.filter_passed = true;
        auto plan = agent_guild_notify_side_effect_plan(in, 2, 0);
        ASSERT_EQ(plan.effects.size(), 3u);
        RecordingSink sink;
        (void)apply_agent_guild_notify_side_effects(plan, sink);
        EXPECT_EQ(sink.calls,
                  std::vector<std::string>({"copy", "filter", "send"}));
    }
}

TEST(ApplyAgentGuildNotifySideEffects, GuildCreateForwardsOrNacks) {
    {
        AgentGuildNotifyValidationInput in;
        in.action = AgentGuildNotifyAction::GuildCreateSyn;
        in.user_found = true;
        in.filter_passed = true;
        in.usable_name_passed = true;
        auto plan = agent_guild_notify_side_effect_plan(in, 7, 0);
        EXPECT_TRUE(plan.forward_to_map);
        RecordingSink sink;
        auto out = apply_agent_guild_notify_side_effects(plan, sink);
        EXPECT_EQ(out.forwards, 1u);
        EXPECT_TRUE(out.forward_flag_consumed);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"forward"}));
    }
    {
        AgentGuildNotifyValidationInput in;
        in.action = AgentGuildNotifyAction::GuildCreateSyn;
        in.user_found = true;
        in.filter_passed = true;
        in.usable_name_passed = false;
        auto plan = agent_guild_notify_side_effect_plan(in, 7, 0);
        EXPECT_TRUE(plan.send_nack);
        RecordingSink sink;
        auto out = apply_agent_guild_notify_side_effects(plan, sink);
        EXPECT_EQ(out.create_nacks, 1u);
        EXPECT_EQ(sink.last_nack_code, LEGACY_EGUILDERR_CREATE_NAME);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"createnack"}));
    }
}

TEST(ApplyAgentGuildNotifySideEffects, GuildNicknameForwardsOrNacks) {
    {
        AgentGuildNotifyValidationInput in;
        in.action = AgentGuildNotifyAction::GuildGiveNicknameSyn;
        in.user_found = true;
        in.usable_name_passed = true;
        in.no_quote_chars = true;
        auto plan = agent_guild_notify_side_effect_plan(in, 7, 0);
        EXPECT_TRUE(plan.forward_to_map);
        RecordingSink sink;
        (void)apply_agent_guild_notify_side_effects(plan, sink);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"forward"}));
    }
    {
        AgentGuildNotifyValidationInput in;
        in.action = AgentGuildNotifyAction::GuildGiveNicknameSyn;
        in.user_found = true;
        in.usable_name_passed = false;
        in.no_quote_chars = true;
        auto plan = agent_guild_notify_side_effect_plan(in, 7, 0);
        EXPECT_TRUE(plan.send_nack);
        RecordingSink sink;
        auto out = apply_agent_guild_notify_side_effects(plan, sink);
        EXPECT_EQ(out.nick_nacks, 1u);
        EXPECT_EQ(sink.last_nack_code, LEGACY_EGUILDERR_NICK_FILTER);
    }
    {
        AgentGuildNotifyValidationInput in;
        in.action = AgentGuildNotifyAction::GuildGiveNicknameSyn;
        in.user_found = true;
        in.usable_name_passed = true;
        in.no_quote_chars = false;
        auto plan = agent_guild_notify_side_effect_plan(in, 7, 0);
        EXPECT_TRUE(plan.send_nack);
    }
}

TEST(ApplyAgentGuildNotifySideEffects, NoUserAndFilteredEmitEmptyPlans) {
    for (const auto action :
         {AgentGuildNotifyAction::MunpaJoinSyn,
          AgentGuildNotifyAction::MunhaNameChangeOrOtherJoinSyn,
          AgentGuildNotifyAction::MunpaDeleteUserAlram,
          AgentGuildNotifyAction::GuildCreateSyn,
          AgentGuildNotifyAction::GuildGiveNicknameSyn}) {
        AgentGuildNotifyValidationInput in;
        in.action = action;
        in.user_found = false;
        auto plan = agent_guild_notify_side_effect_plan(in, 7, 0);
        EXPECT_FALSE(plan.dispatched);
        EXPECT_FALSE(plan.forward_to_map);
        EXPECT_FALSE(plan.send_nack);
        EXPECT_TRUE(plan.effects.empty());
    }

    AgentGuildNotifyValidationInput filtered;
    filtered.action = AgentGuildNotifyAction::MunpaJoinSyn;
    filtered.user_found = true;
    filtered.filter_passed = false;
    auto plan = agent_guild_notify_side_effect_plan(filtered, 7, 0);
    EXPECT_TRUE(plan.effects.empty());
}

TEST(ApplyAgentGuildNotifySideEffects, EmptyPlanIsNoOp) {
    mxh::server::AgentGuildNotifySideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_agent_guild_notify_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.copies, 0u);
    EXPECT_EQ(out.filters, 0u);
    EXPECT_EQ(out.note_sends, 0u);
    EXPECT_EQ(out.join_alarms, 0u);
    EXPECT_EQ(out.munha_alarms, 0u);
    EXPECT_EQ(out.create_nacks, 0u);
    EXPECT_EQ(out.nick_nacks, 0u);
    EXPECT_EQ(out.forwards, 0u);
    EXPECT_FALSE(out.dispatched_flag_consumed);
    EXPECT_FALSE(out.forward_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}
