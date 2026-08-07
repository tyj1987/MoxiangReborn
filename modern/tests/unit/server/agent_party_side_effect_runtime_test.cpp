// agent_party_side_effect_runtime_test.cpp
//
// Verifies apply_agent_party_side_effects() (the runtime orchestrator
// for the legacy agent party side-effect chains) walks the data-plane
// plan and dispatches each entry: broadcast / map / user / nack /
// error / client routing in legacy order.

#include <mxh/server/agent_party_side_effect.hpp>
#include <mxh/server/agent_party_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::AgentPartyAction;
using mxh::server::AgentPartySideEffectSink;
using mxh::server::AgentPartyValidationInput;
using mxh::server::LEGACY_EERR_REQUEST_NOT_MASTER;
using mxh::server::LEGACY_PARTY_ERROR;
using mxh::server::LEGACY_PARTY_REQUEST_CONSENT_NACK;
using mxh::server::LEGACY_PARTY_REQUEST_REFUSAL_NACK;
using mxh::server::apply_agent_party_side_effects;
using mxh::server::agent_party_side_effect_plan;

class RecordingSink final : public AgentPartySideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_object_id = 0;
    std::uint32_t last_alternate_id = 0;
    std::uint32_t last_error_code = 0;
    std::uint8_t last_protocol = 0;

    void broadcast_to_other_map_servers(std::uint32_t object_id) override {
        calls.push_back("bcast");
        last_object_id = object_id;
    }
    void send_to_target_map_server(std::uint32_t object_id,
                                   std::uint32_t alternate_object_id,
                                   std::uint8_t protocol) override {
        calls.push_back("map");
        last_object_id = object_id;
        last_alternate_id = alternate_object_id;
        last_protocol = protocol;
    }
    void send_to_target_user(std::uint32_t object_id,
                             std::uint32_t alternate_object_id,
                             std::uint8_t protocol) override {
        calls.push_back("user");
        last_object_id = object_id;
        last_alternate_id = alternate_object_id;
        last_protocol = protocol;
    }
    void send_consent_nack(std::uint32_t alternate_object_id,
                           std::uint8_t protocol) override {
        calls.push_back("consentnack");
        last_alternate_id = alternate_object_id;
        last_protocol = protocol;
    }
    void send_refusal_nack(std::uint32_t alternate_object_id,
                           std::uint8_t protocol) override {
        calls.push_back("refusalnack");
        last_alternate_id = alternate_object_id;
        last_protocol = protocol;
    }
    void send_party_error(std::uint32_t object_id,
                          std::uint32_t error_code,
                          std::uint8_t protocol) override {
        calls.push_back("err");
        last_object_id = object_id;
        last_error_code = error_code;
        last_protocol = protocol;
    }
    void send_to_client(std::uint32_t object_id,
                        std::uint8_t protocol) override {
        calls.push_back("client");
        last_object_id = object_id;
        last_protocol = protocol;
    }
};

}  // namespace

TEST(ApplyAgentPartySideEffects, AllTenMapNotificationsBroadcastToOtherMaps) {
    const AgentPartyAction actions[] = {
        AgentPartyAction::NotifyAddToMapServer,
        AgentPartyAction::NotifyDeleteToMapServer,
        AgentPartyAction::NotifyChangeMasterToMapServer,
        AgentPartyAction::NotifyBreakupToMapServer,
        AgentPartyAction::NotifyBanToMapServer,
        AgentPartyAction::NotifyMemberLoginToMapServer,
        AgentPartyAction::NotifyMemberLogoutToMapServer,
        AgentPartyAction::NotifyMemberLoginMessage,
        AgentPartyAction::NotifyCreateToMapServer,
        AgentPartyAction::NotifyMemberLevel,
    };
    for (const auto action : actions) {
        AgentPartyValidationInput in;
        in.action = action;
        auto plan = agent_party_side_effect_plan(in, 7, 0);
        EXPECT_TRUE(plan.broadcast_to_maps);
        ASSERT_EQ(plan.effects.size(), 1u);

        RecordingSink sink;
        auto out = apply_agent_party_side_effects(plan, sink);
        EXPECT_EQ(out.broadcasts, 1u);
        EXPECT_TRUE(out.broadcast_flag_consumed);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"bcast"}));
    }
}

TEST(ApplyAgentPartySideEffects, ConsentAckRoutes) {
    {
        AgentPartyValidationInput in;
        in.action = AgentPartyAction::RequestConsentAck;
        in.object_found = true;
        auto plan = agent_party_side_effect_plan(
            in, /*object_id=*/10, /*alternate=*/20);
        EXPECT_TRUE(plan.forward_to_map);
        RecordingSink sink;
        auto out = apply_agent_party_side_effects(plan, sink);
        EXPECT_EQ(out.map_forwards, 1u);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"map"}));
        EXPECT_EQ(sink.last_object_id, 10u);
        EXPECT_EQ(sink.last_alternate_id, 20u);
        EXPECT_EQ(sink.last_protocol, 54u);
    }
    {
        AgentPartyValidationInput in;
        in.action = AgentPartyAction::RequestConsentAck;
        in.object_found = false;
        in.alternate_found = true;
        auto plan = agent_party_side_effect_plan(in, 10, 20);
        EXPECT_TRUE(plan.send_nack);
        RecordingSink sink;
        auto out = apply_agent_party_side_effects(plan, sink);
        EXPECT_EQ(out.consent_nacks, 1u);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"consentnack"}));
        EXPECT_EQ(sink.last_alternate_id, 20u);
        EXPECT_EQ(sink.last_protocol,
                  LEGACY_PARTY_REQUEST_CONSENT_NACK);
    }
}

TEST(ApplyAgentPartySideEffects, RefusalAckRoutes) {
    {
        AgentPartyValidationInput in;
        in.action = AgentPartyAction::RequestRefusalAck;
        in.object_found = true;
        auto plan = agent_party_side_effect_plan(in, 10, 20);
        RecordingSink sink;
        (void)apply_agent_party_side_effects(plan, sink);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"user"}));
        EXPECT_EQ(sink.last_protocol, 57u);
    }
    {
        AgentPartyValidationInput in;
        in.action = AgentPartyAction::RequestRefusalAck;
        in.object_found = false;
        in.alternate_found = true;
        auto plan = agent_party_side_effect_plan(in, 10, 20);
        RecordingSink sink;
        (void)apply_agent_party_side_effects(plan, sink);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"refusalnack"}));
        EXPECT_EQ(sink.last_protocol,
                  LEGACY_PARTY_REQUEST_REFUSAL_NACK);
    }
}

TEST(ApplyAgentPartySideEffects, ErrorAndMatchingInfoRoute) {
    {
        AgentPartyValidationInput in;
        in.action = AgentPartyAction::Error;
        in.object_found = true;
        auto plan = agent_party_side_effect_plan(in, 10, 0);
        RecordingSink sink;
        (void)apply_agent_party_side_effects(plan, sink);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"user"}));
        EXPECT_EQ(sink.last_protocol, LEGACY_PARTY_ERROR);
    }
    {
        AgentPartyValidationInput in;
        in.action = AgentPartyAction::MatchingInfo;
        auto plan = agent_party_side_effect_plan(in, 10, 0);
        EXPECT_TRUE(plan.forward_to_client);
        RecordingSink sink;
        auto out = apply_agent_party_side_effects(plan, sink);
        EXPECT_EQ(out.client_forwards, 1u);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"client"}));
        EXPECT_EQ(sink.last_protocol, 60u);
    }
}

TEST(ApplyAgentPartySideEffects, MasterToRequestSynRoutes) {
    {
        AgentPartyValidationInput in;
        in.action = AgentPartyAction::MasterToRequestSyn;
        in.master_found = true;
        auto plan = agent_party_side_effect_plan(
            in, /*object_id=*/30, /*alternate=*/40);
        EXPECT_TRUE(plan.forward_to_map);
        RecordingSink sink;
        (void)apply_agent_party_side_effects(plan, sink);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"map"}));
        EXPECT_EQ(sink.last_object_id, 40u);
        EXPECT_EQ(sink.last_alternate_id, 30u);
        EXPECT_EQ(sink.last_protocol, 51u);
    }
    {
        AgentPartyValidationInput in;
        in.action = AgentPartyAction::MasterToRequestSyn;
        in.master_found = false;
        in.requester_found = true;
        auto plan = agent_party_side_effect_plan(in, 30, 40);
        EXPECT_TRUE(plan.send_nack);
        RecordingSink sink;
        auto out = apply_agent_party_side_effects(plan, sink);
        EXPECT_EQ(out.party_errors, 1u);
        EXPECT_EQ(sink.calls, std::vector<std::string>({"err"}));
        EXPECT_EQ(sink.last_object_id, 30u);
        EXPECT_EQ(sink.last_error_code, LEGACY_EERR_REQUEST_NOT_MASTER);
        EXPECT_EQ(sink.last_protocol, LEGACY_PARTY_ERROR);
    }
}

TEST(ApplyAgentPartySideEffects, NoTargetEmitsEmptyPlan) {
    const AgentPartyAction actions[] = {
        AgentPartyAction::RequestConsentAck,
        AgentPartyAction::RequestRefusalAck,
        AgentPartyAction::Error,
        AgentPartyAction::MasterToRequestSyn,
    };
    for (const auto action : actions) {
        AgentPartyValidationInput in;
        in.action = action;
        auto plan = agent_party_side_effect_plan(in, 7, 0);
        EXPECT_FALSE(plan.dispatched);
        EXPECT_TRUE(plan.effects.empty());
    }
}

TEST(ApplyAgentPartySideEffects, EmptyPlanIsNoOp) {
    mxh::server::AgentPartySideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_agent_party_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.broadcasts, 0u);
    EXPECT_EQ(out.map_forwards, 0u);
    EXPECT_EQ(out.user_forwards, 0u);
    EXPECT_EQ(out.consent_nacks, 0u);
    EXPECT_EQ(out.refusal_nacks, 0u);
    EXPECT_EQ(out.party_errors, 0u);
    EXPECT_EQ(out.client_forwards, 0u);
    EXPECT_FALSE(out.dispatched_flag_consumed);
    EXPECT_FALSE(out.broadcast_flag_consumed);
    EXPECT_FALSE(out.map_flag_consumed);
    EXPECT_FALSE(out.client_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}
