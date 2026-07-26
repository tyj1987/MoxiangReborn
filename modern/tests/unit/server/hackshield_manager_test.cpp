// hackshield_manager_test.cpp - Phase 6.3 HackShieldManager tests.

#include "mxh/server/hackshield_manager.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::ANTICPSVR_CHECK_ALL;
using mxh::server::ANTICPSVR_CHECK_GAME_FILE;
using mxh::server::ANTICPSVR_CHECK_GAME_MEMORY;
using mxh::server::ANTICPSVR_CHECK_HACKSHIELD_FILE;
using mxh::server::HACKSHIELD_ACK_SIZE;
using mxh::server::HACKSHIELD_CATEGORY;
using mxh::server::HACKSHIELD_GUID_ACK_SIZE;
using mxh::server::HACKSHIELD_GUID_REQ_SIZE;
using mxh::server::HACKSHIELD_REQ_SIZE;
using mxh::server::HACKSHIELD_SUPERUSER_LEVEL;
using mxh::server::HackShieldActionKind;
using mxh::server::HackShieldProtocol;
using mxh::server::HackShieldUserState;
using mxh::server::parse_hackshield_message;
using mxh::server::send_guid_req;
using mxh::server::send_hackshield_req;

HackShieldUserState regular_user() {
    HackShieldUserState user;
    user.dwConnectionIndex = 42u;
    user.UserLevel = HACKSHIELD_SUPERUSER_LEVEL;
    return user;
}

} // namespace

TEST(HackShieldConstants, MessageSizesMatchVendorHeader) {
    EXPECT_EQ(HACKSHIELD_REQ_SIZE, 160u);
    EXPECT_EQ(HACKSHIELD_ACK_SIZE, 56u);
    EXPECT_EQ(HACKSHIELD_GUID_REQ_SIZE, 20u);
    EXPECT_EQ(HACKSHIELD_GUID_ACK_SIZE, 20u);
}

TEST(HackShieldConstants, CategoryAndProtocolsMatchLegacy) {
    EXPECT_EQ(HACKSHIELD_CATEGORY, 67u);
    EXPECT_EQ(static_cast<int>(HackShieldProtocol::GuidReq), 0);
    EXPECT_EQ(static_cast<int>(HackShieldProtocol::GuidAck), 1);
    EXPECT_EQ(static_cast<int>(HackShieldProtocol::Req), 2);
    EXPECT_EQ(static_cast<int>(HackShieldProtocol::Ack), 3);
    EXPECT_EQ(static_cast<int>(HackShieldProtocol::Disconnect), 4);
}

TEST(HackShieldConstants, CheckOptionsMatchVendorHeader) {
    EXPECT_EQ(ANTICPSVR_CHECK_GAME_MEMORY, 0x1u);
    EXPECT_EQ(ANTICPSVR_CHECK_HACKSHIELD_FILE, 0x2u);
    EXPECT_EQ(ANTICPSVR_CHECK_GAME_FILE, 0x4u);
    EXPECT_EQ(ANTICPSVR_CHECK_ALL, 0x7u);
}

TEST(HackShieldGuidReq, PrivilegedLevelBelowSuperuserIsSkipped) {
    auto user = regular_user();
    user.UserLevel = HACKSHIELD_SUPERUSER_LEVEL - 1u;
    const auto action = send_guid_req(user, true);
    EXPECT_EQ(action.Kind, HackShieldActionKind::None);
    EXPECT_EQ(user.m_bHSCheck, 0u);
}

TEST(HackShieldGuidReq, SuccessSendsZeroedGuidRequestAndSetsGraceState) {
    auto user = regular_user();
    const auto action = send_guid_req(user, true);
    ASSERT_EQ(action.Kind, HackShieldActionKind::Send);
    EXPECT_EQ(action.Packet.Category, HACKSHIELD_CATEGORY);
    EXPECT_EQ(action.Packet.Protocol, HackShieldProtocol::GuidReq);
    EXPECT_EQ(action.Packet.PayloadSize, HACKSHIELD_GUID_REQ_SIZE);
    for (std::size_t i = 0; i < action.Packet.PayloadSize; ++i) {
        EXPECT_EQ(action.Packet.Payload[i], 0u);
    }
    EXPECT_EQ(user.m_bHSCheck, 2u);
}

TEST(HackShieldGuidReq, VendorFailureDoesNotSendOrChangeState) {
    auto user = regular_user();
    const auto action = send_guid_req(user, false);
    EXPECT_EQ(action.Kind, HackShieldActionKind::None);
    EXPECT_EQ(user.m_bHSCheck, 0u);
}

TEST(HackShieldSendReq, GraceStateBecomesWaitingWithoutSending) {
    auto user = regular_user();
    user.m_bHSCheck = 2u;
    const auto action = send_hackshield_req(user, true);
    EXPECT_EQ(action.Kind, HackShieldActionKind::None);
    EXPECT_EQ(user.m_bHSCheck, 1u);
}

TEST(HackShieldSendReq, WaitingStateDisconnectsOnNextTick) {
    auto user = regular_user();
    user.m_bHSCheck = 1u;
    const auto action = send_hackshield_req(user, true);
    EXPECT_EQ(action.Kind, HackShieldActionKind::Disconnect);
    EXPECT_EQ(action.Packet.Protocol, HackShieldProtocol::Disconnect);
    EXPECT_EQ(user.m_bHSCheck, 1u);
}

TEST(HackShieldSendReq, IdleStateSendsGameMemoryRequest) {
    auto user = regular_user();
    const auto action = send_hackshield_req(user, true);
    EXPECT_EQ(action.Kind, HackShieldActionKind::Send);
    EXPECT_EQ(action.Packet.Protocol, HackShieldProtocol::Req);
    EXPECT_EQ(action.Packet.PayloadSize, HACKSHIELD_REQ_SIZE);
    EXPECT_EQ(action.CheckOption, ANTICPSVR_CHECK_GAME_MEMORY);
    EXPECT_EQ(user.m_bHSCheck, 1u);
}

TEST(HackShieldSendReq, VendorFailureLeavesIdleState) {
    auto user = regular_user();
    const auto action = send_hackshield_req(user, false);
    EXPECT_EQ(action.Kind, HackShieldActionKind::None);
    EXPECT_EQ(user.m_bHSCheck, 0u);
}

TEST(HackShieldGuidAck, SuccessfulAnalysisSendsInitialAllCheckRequest) {
    auto user = regular_user();
    user.m_bHSCheck = 2u;
    const auto action = parse_hackshield_message(
        &user, HackShieldProtocol::GuidAck, true, true);
    EXPECT_EQ(action.Kind, HackShieldActionKind::Send);
    EXPECT_EQ(action.Packet.Protocol, HackShieldProtocol::Req);
    EXPECT_EQ(action.Packet.PayloadSize, HACKSHIELD_REQ_SIZE);
    EXPECT_EQ(action.CheckOption, ANTICPSVR_CHECK_ALL);
    EXPECT_EQ(user.m_bHSCheck, 2u);
}

TEST(HackShieldGuidAck, AnalysisFailureClearsStateAndDisconnects) {
    auto user = regular_user();
    user.m_bHSCheck = 2u;
    const auto action = parse_hackshield_message(
        &user, HackShieldProtocol::GuidAck, false, true);
    EXPECT_EQ(action.Kind, HackShieldActionKind::Disconnect);
    EXPECT_EQ(user.m_bHSCheck, 0u);
}

TEST(HackShieldGuidAck, RequestCreationFailureLeavesClearedState) {
    auto user = regular_user();
    user.m_bHSCheck = 2u;
    const auto action = parse_hackshield_message(
        &user, HackShieldProtocol::GuidAck, true, false);
    EXPECT_EQ(action.Kind, HackShieldActionKind::None);
    EXPECT_EQ(user.m_bHSCheck, 0u);
}

TEST(HackShieldAck, SuccessfulAnalysisClearsWaitingState) {
    auto user = regular_user();
    user.m_bHSCheck = 1u;
    const auto action = parse_hackshield_message(
        &user, HackShieldProtocol::Ack, true);
    EXPECT_EQ(action.Kind, HackShieldActionKind::None);
    EXPECT_EQ(user.m_bHSCheck, 0u);
}

TEST(HackShieldAck, AnalysisFailureClearsStateAndDisconnects) {
    auto user = regular_user();
    user.m_bHSCheck = 1u;
    const auto action = parse_hackshield_message(
        &user, HackShieldProtocol::Ack, false);
    EXPECT_EQ(action.Kind, HackShieldActionKind::Disconnect);
    EXPECT_EQ(action.Packet.Protocol, HackShieldProtocol::Disconnect);
    EXPECT_EQ(user.m_bHSCheck, 0u);
}

TEST(HackShieldParse, MissingUserIsIgnored) {
    const auto action = parse_hackshield_message(
        nullptr, HackShieldProtocol::GuidAck, false, false);
    EXPECT_EQ(action.Kind, HackShieldActionKind::None);
}

TEST(HackShieldParse, UnknownOrServerOnlyProtocolIsIgnored) {
    auto user = regular_user();
    user.m_bHSCheck = 2u;
    const auto action = parse_hackshield_message(
        &user, HackShieldProtocol::Req, false, false);
    EXPECT_EQ(action.Kind, HackShieldActionKind::None);
    EXPECT_EQ(user.m_bHSCheck, 2u);
}

TEST(HackShieldBoundary, SuperuserLevelItselfIsChecked) {
    auto user = regular_user();
    user.UserLevel = HACKSHIELD_SUPERUSER_LEVEL;
    EXPECT_EQ(send_guid_req(user, true).Kind, HackShieldActionKind::Send);
}

TEST(HackShieldBoundary, NormalUserLevelIsChecked) {
    auto user = regular_user();
    user.UserLevel = HACKSHIELD_SUPERUSER_LEVEL + 1u;
    EXPECT_EQ(send_hackshield_req(user, true).Kind, HackShieldActionKind::Send);
}