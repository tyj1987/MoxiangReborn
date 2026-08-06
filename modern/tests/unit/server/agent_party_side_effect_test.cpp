#include <array>
#include <cstddef>

#include <mxh/server/agent_party_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

AgentPartyValidationInput broadcast_input(AgentPartyAction action) {
    AgentPartyValidationInput input{};
    input.action = action;
    return input;
}

AgentPartyValidationInput consent_ok() {
    AgentPartyValidationInput input{};
    input.action = AgentPartyAction::RequestConsentAck;
    input.object_found = true;
    return input;
}

AgentPartyValidationInput refusal_ok() {
    AgentPartyValidationInput input{};
    input.action = AgentPartyAction::RequestRefusalAck;
    input.object_found = true;
    return input;
}

AgentPartyValidationInput error_ok() {
    AgentPartyValidationInput input{};
    input.action = AgentPartyAction::Error;
    input.object_found = true;
    return input;
}

AgentPartyValidationInput master_request_ok() {
    AgentPartyValidationInput input{};
    input.action = AgentPartyAction::MasterToRequestSyn;
    input.master_found = true;
    return input;
}

TEST(AgentPartyOutcome, AllTenMapNotificationsBroadcastToOtherMaps) {
    const std::array actions = {
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
        EXPECT_EQ(classify_agent_party_outcome(broadcast_input(action)),
                  AgentPartyOutcome::BroadcastedToOtherMaps);
    }
}

TEST(AgentPartyOutcome, ConsentAckForwardsToTargetMap) {
    EXPECT_EQ(classify_agent_party_outcome(consent_ok()),
              AgentPartyOutcome::ForwardedToTargetMap);
}

TEST(AgentPartyOutcome, ConsentAckFallsBackToNack) {
    auto input = consent_ok();
    input.object_found = false;
    input.alternate_found = true;
    EXPECT_EQ(classify_agent_party_outcome(input),
              AgentPartyOutcome::SentConsentNack);
}

TEST(AgentPartyOutcome, ConsentAckWithoutEitherUserIsDropped) {
    auto input = consent_ok();
    input.object_found = false;
    EXPECT_EQ(classify_agent_party_outcome(input), AgentPartyOutcome::NoTarget);
}

TEST(AgentPartyOutcome, RefusalAckForwardsToTargetUser) {
    EXPECT_EQ(classify_agent_party_outcome(refusal_ok()),
              AgentPartyOutcome::ForwardedToTargetUser);
}

TEST(AgentPartyOutcome, RefusalAckFallsBackToNack) {
    auto input = refusal_ok();
    input.object_found = false;
    input.alternate_found = true;
    EXPECT_EQ(classify_agent_party_outcome(input),
              AgentPartyOutcome::SentRefusalNack);
}

TEST(AgentPartyOutcome, RefusalAckWithoutEitherUserIsDropped) {
    auto input = refusal_ok();
    input.object_found = false;
    EXPECT_EQ(classify_agent_party_outcome(input), AgentPartyOutcome::NoTarget);
}

TEST(AgentPartyOutcome, ErrorForwardsWhenTargetUserFound) {
    EXPECT_EQ(classify_agent_party_outcome(error_ok()),
              AgentPartyOutcome::ForwardedToTargetUser);
}

TEST(AgentPartyOutcome, ErrorWithoutTargetUserIsDropped) {
    auto input = error_ok();
    input.object_found = false;
    EXPECT_EQ(classify_agent_party_outcome(input), AgentPartyOutcome::NoTarget);
}

TEST(AgentPartyOutcome, MatchingInfoUsesDefaultClientForward) {
    EXPECT_EQ(classify_agent_party_outcome(
                  broadcast_input(AgentPartyAction::MatchingInfo)),
              AgentPartyOutcome::ForwardedToClient);
}

TEST(AgentPartyOutcome, MasterRequestForwardsToMasterMap) {
    EXPECT_EQ(classify_agent_party_outcome(master_request_ok()),
              AgentPartyOutcome::ForwardedToTargetMap);
}

TEST(AgentPartyOutcome, MasterRequestSendsNotMasterErrorToRequester) {
    auto input = master_request_ok();
    input.master_found = false;
    input.requester_found = true;
    EXPECT_EQ(classify_agent_party_outcome(input),
              AgentPartyOutcome::SentPartyError);
}

TEST(AgentPartyOutcome, MasterRequestWithoutMasterOrRequesterIsDropped) {
    auto input = master_request_ok();
    input.master_found = false;
    EXPECT_EQ(classify_agent_party_outcome(input), AgentPartyOutcome::NoTarget);
}

TEST(AgentPartyPlan, NotificationEmitsOneBroadcastEffect) {
    const auto plan = agent_party_side_effect_plan(
        broadcast_input(AgentPartyAction::NotifyMemberLevel), 10u, 0u);
    ASSERT_TRUE(plan.dispatched);
    ASSERT_TRUE(plan.broadcast_to_maps);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              AgentPartySideEffectKind::BroadcastToOtherMapServers);
    EXPECT_EQ(plan.effects[0].object_id, 10u);
}

TEST(AgentPartyPlan, ConsentAckForwardsOriginalPacketToTargetMap) {
    const auto plan = agent_party_side_effect_plan(consent_ok(), 10u, 20u);
    ASSERT_TRUE(plan.dispatched);
    ASSERT_TRUE(plan.forward_to_map);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentPartySideEffectKind::SendToTargetMapServer);
    EXPECT_EQ(plan.effects[0].object_id, 10u);
    EXPECT_EQ(plan.effects[0].protocol, 54u);
}

TEST(AgentPartyPlan, ConsentAckSendsNackToFallbackUser) {
    auto input = consent_ok();
    input.object_found = false;
    input.alternate_found = true;
    const auto plan = agent_party_side_effect_plan(input, 10u, 20u);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentPartySideEffectKind::SendConsentNack);
    EXPECT_EQ(plan.effects[0].object_id, 20u);
    EXPECT_EQ(plan.effects[0].protocol, LEGACY_PARTY_REQUEST_CONSENT_NACK);
}

TEST(AgentPartyPlan, RefusalAckForwardsOriginalPacketToTargetUser) {
    const auto plan = agent_party_side_effect_plan(refusal_ok(), 10u, 20u);
    ASSERT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentPartySideEffectKind::SendToTargetUser);
    EXPECT_EQ(plan.effects[0].object_id, 10u);
    EXPECT_EQ(plan.effects[0].protocol, 57u);
}

TEST(AgentPartyPlan, RefusalAckSendsNackToFallbackUser) {
    auto input = refusal_ok();
    input.object_found = false;
    input.alternate_found = true;
    const auto plan = agent_party_side_effect_plan(input, 10u, 20u);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentPartySideEffectKind::SendRefusalNack);
    EXPECT_EQ(plan.effects[0].object_id, 20u);
    EXPECT_EQ(plan.effects[0].protocol, LEGACY_PARTY_REQUEST_REFUSAL_NACK);
}

TEST(AgentPartyPlan, ErrorForwardsOriginalPacketToTargetUser) {
    const auto plan = agent_party_side_effect_plan(error_ok(), 10u, 0u);
    ASSERT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentPartySideEffectKind::SendToTargetUser);
    EXPECT_EQ(plan.effects[0].object_id, 10u);
    EXPECT_EQ(plan.effects[0].protocol, LEGACY_PARTY_ERROR);
}

TEST(AgentPartyPlan, MatchingInfoUsesClientPassthrough) {
    const auto plan = agent_party_side_effect_plan(
        broadcast_input(AgentPartyAction::MatchingInfo), 10u, 0u);
    ASSERT_TRUE(plan.dispatched);
    ASSERT_TRUE(plan.forward_to_client);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentPartySideEffectKind::SendToClient);
    EXPECT_EQ(plan.effects[0].protocol, 60u);
}

TEST(AgentPartyPlan, MasterRequestForwardsToMasterMap) {
    const auto plan = agent_party_side_effect_plan(master_request_ok(), 10u, 20u);
    ASSERT_TRUE(plan.dispatched);
    ASSERT_TRUE(plan.forward_to_map);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentPartySideEffectKind::SendToTargetMapServer);
    EXPECT_EQ(plan.effects[0].object_id, 20u);
    EXPECT_EQ(plan.effects[0].alternate_object_id, 10u);
    EXPECT_EQ(plan.effects[0].protocol, 51u);
}

TEST(AgentPartyPlan, MasterRequestSendsNotMasterError) {
    auto input = master_request_ok();
    input.master_found = false;
    input.requester_found = true;
    const auto plan = agent_party_side_effect_plan(input, 10u, 20u);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentPartySideEffectKind::SendPartyError);
    EXPECT_EQ(plan.effects[0].object_id, 10u);
    EXPECT_EQ(plan.effects[0].error_code, LEGACY_EERR_REQUEST_NOT_MASTER);
    EXPECT_EQ(plan.effects[0].protocol, LEGACY_PARTY_ERROR);
}

TEST(AgentPartyPlan, MissingTargetsEmitNoEffects) {
    auto consent = consent_ok();
    consent.object_found = false;
    const auto consent_plan = agent_party_side_effect_plan(consent, 10u, 20u);
    EXPECT_TRUE(consent_plan.effects.empty());
    auto master = master_request_ok();
    master.master_found = false;
    const auto master_plan = agent_party_side_effect_plan(master, 10u, 20u);
    EXPECT_TRUE(master_plan.effects.empty());
}

TEST(AgentPartyPlan, PlanIsIdempotent) {
    const auto input = consent_ok();
    const auto first = agent_party_side_effect_plan(input, 10u, 20u);
    const auto second = agent_party_side_effect_plan(input, 10u, 20u);
    EXPECT_EQ(first.dispatched, second.dispatched);
    EXPECT_EQ(first.broadcast_to_maps, second.broadcast_to_maps);
    EXPECT_EQ(first.forward_to_map, second.forward_to_map);
    EXPECT_EQ(first.forward_to_client, second.forward_to_client);
    EXPECT_EQ(first.send_nack, second.send_nack);
    ASSERT_EQ(first.effects.size(), second.effects.size());
    for (std::size_t index = 0; index < first.effects.size(); ++index) {
        EXPECT_EQ(first.effects[index].kind, second.effects[index].kind);
        EXPECT_EQ(first.effects[index].object_id, second.effects[index].object_id);
        EXPECT_EQ(first.effects[index].protocol, second.effects[index].protocol);
    }
}
