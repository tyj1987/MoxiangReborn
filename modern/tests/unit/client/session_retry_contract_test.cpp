// Pins the existing reconnect policy: a dropped session is a full re-login.
// There is no auto-resume. The Win32 host in MoxianClient/main.cpp applies
// the Title transition and clears the login form; CEngine.hpp includes
// objbase.h, so those translation units are not built in this Linux target.
#include "AgentSession.hpp"
#include "CCharMake.hpp"
#include "CCharSelectState.hpp"
#include "CInGameState.hpp"
#include "CLoginState.hpp"

#include "mxh/client/session_retry.hpp"
#include "mxh/net/connect_retry.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <vector>

namespace {

using clock = std::chrono::steady_clock;

struct FakeClock {
    clock::time_point now{};
    std::vector<std::chrono::milliseconds> sleeps;

    clock::time_point operator()() const { return now; }
    void sleep(std::chrono::milliseconds delay) {
        sleeps.push_back(delay);
        now += delay;
    }
};

}  // namespace

TEST(SessionRetryContract, ConnectRetriesUntilDeadlineThenFails) {
    FakeClock clock;
    int attempts = 0;
    const auto budget = mxh::client::kLoginConnectTimeout;
    const bool connected = mxh::net::retry_connect_until(
        budget,
        [&] { return clock.now; },
        [&](std::chrono::milliseconds delay) { clock.sleep(delay); },
        [&] {
            ++attempts;
            return false;
        });
    EXPECT_FALSE(connected);
    EXPECT_EQ(clock.now.time_since_epoch(), budget);
    EXPECT_EQ(attempts, static_cast<int>(budget / mxh::net::kConnectRetryInterval) + 1);
    ASSERT_EQ(clock.sleeps.size(), static_cast<std::size_t>(attempts - 1));
    for (const auto delay : clock.sleeps) {
        EXPECT_EQ(delay, mxh::net::kConnectRetryInterval);
    }
}

TEST(SessionRetryContract, ConnectSucceedsBeforeDeadline) {
    FakeClock clock;
    int attempts = 0;
    const bool connected = mxh::net::retry_connect_until(
        mxh::client::kAgentConnectTimeout,
        [&] { return clock.now; },
        [&](std::chrono::milliseconds delay) { clock.sleep(delay); },
        [&] { return ++attempts == 3; });
    EXPECT_TRUE(connected);
    EXPECT_EQ(attempts, 3);
    EXPECT_EQ(clock.sleeps.size(), 2u);
    EXPECT_LT(clock.now.time_since_epoch(), mxh::client::kAgentConnectTimeout);
}

TEST(SessionRetryContract, LoginAndAgentUseThreeSecondConnectBudget) {
    EXPECT_EQ(mxh::net::kConnectRetryInterval, std::chrono::milliseconds(50));
    EXPECT_EQ(mxh::client::kLoginConnectTimeout, std::chrono::milliseconds(3000));
    EXPECT_EQ(mxh::client::kAgentConnectTimeout, std::chrono::milliseconds(3000));
    EXPECT_EQ(mxh::client::CLoginState::kConnectTimeout, mxh::client::kLoginConnectTimeout);
    EXPECT_EQ(mxh::client::AgentSession::kConnectTimeout, mxh::client::kAgentConnectTimeout);
}

TEST(SessionRetryContract, LoginAckTimeoutIsTenSecondsAndFailsWithReason) {
    EXPECT_EQ(mxh::client::CLoginState::kAckTimeout, std::chrono::seconds(10));
    EXPECT_EQ(mxh::client::kLoginAckTimeout, mxh::client::CLoginState::kAckTimeout);
    const auto start = clock::time_point(std::chrono::seconds(100));
    const auto deadline = start + mxh::client::kLoginAckTimeout;
    EXPECT_FALSE(mxh::client::ack_wait_expired(
        false, false, deadline, deadline - std::chrono::milliseconds(1)));
    EXPECT_TRUE(mxh::client::ack_wait_expired(false, false, deadline, deadline));
    EXPECT_FALSE(mxh::client::ack_wait_expired(
        true, false, deadline, deadline + std::chrono::seconds(1)));
    EXPECT_FALSE(mxh::client::ack_wait_expired(
        false, true, deadline, deadline + std::chrono::seconds(1)));
    EXPECT_STREQ(mxh::client::kLoginAckTimeoutReason,
                 "LoginAck timeout (no response from LoginServer)");
}

TEST(SessionRetryContract, CharSelectTimeoutsAreTenSecondsAndFailWithReason) {
    EXPECT_EQ(mxh::client::CCharSelectState::kAckTimeout, std::chrono::seconds(10));
    EXPECT_EQ(mxh::client::kCharSelectAckTimeout, mxh::client::CCharSelectState::kAckTimeout);
    const auto start = clock::time_point(std::chrono::seconds(50));
    const auto deadline = start + mxh::client::kCharSelectAckTimeout;
    EXPECT_TRUE(mxh::client::ack_wait_expired(false, false, deadline, deadline));
    EXPECT_FALSE(mxh::client::ack_wait_expired(false, true, deadline, deadline));
    EXPECT_STREQ(mxh::client::kCharacterListAckTimeoutReason,
                 "CharacterListAck timeout (no response from AgentServer)");
    EXPECT_STREQ(mxh::client::kCharacterSelectAckTimeoutReason,
                 "CharacterSelectAck timeout (no response from AgentServer)");
}

TEST(SessionRetryContract, CharMakeAckTimeoutIsTenSecondsAndFailsWithReason) {
    EXPECT_EQ(mxh::client::CCharMake::kAckTimeout, std::chrono::seconds(10));
    EXPECT_EQ(mxh::client::kCharMakeAckTimeout, mxh::client::CCharMake::kAckTimeout);
    const auto deadline = clock::time_point(std::chrono::seconds(20)) +
                          mxh::client::kCharMakeAckTimeout;
    EXPECT_TRUE(mxh::client::ack_wait_expired(false, false, deadline, deadline));
    EXPECT_STREQ(mxh::client::kCharacterMakeAckTimeoutReason,
                 "CharacterMakeAck timeout (no response from AgentServer)");
}

TEST(SessionRetryContract, GameInAckTimeoutIsTenSecondsAndFailsWithReason) {
    EXPECT_EQ(mxh::client::CInGameState::kGameInAckTimeoutMs, 10000u);
    EXPECT_EQ(mxh::client::kGameInAckTimeoutMs, mxh::client::CInGameState::kGameInAckTimeoutMs);
    EXPECT_FALSE(mxh::client::game_in_ack_expired(
        true, 1000, 1000 + mxh::client::kGameInAckTimeoutMs - 1,
        mxh::client::kGameInAckTimeoutMs));
    EXPECT_TRUE(mxh::client::game_in_ack_expired(
        true, 1000, 1000 + mxh::client::kGameInAckTimeoutMs,
        mxh::client::kGameInAckTimeoutMs));
    EXPECT_FALSE(mxh::client::game_in_ack_expired(
        false, 1000, 1000 + mxh::client::kGameInAckTimeoutMs,
        mxh::client::kGameInAckTimeoutMs));
    EXPECT_STREQ(mxh::client::kGameInAckTimeoutReason,
                 "GameInAck timeout (no response from MapServer)");
}

TEST(SessionRetryContract, GameInSynResendsWhileSendFails) {
    const auto failed = mxh::client::tick_game_in_syn(true, false, false);
    EXPECT_TRUE(failed.attempted);
    EXPECT_FALSE(failed.sent);
    const auto sent = mxh::client::tick_game_in_syn(true, failed.sent, true);
    EXPECT_TRUE(sent.attempted);
    EXPECT_TRUE(sent.sent);
    const auto held = mxh::client::tick_game_in_syn(true, sent.sent, false);
    EXPECT_FALSE(held.attempted);
    EXPECT_TRUE(held.sent);
    EXPECT_FALSE(mxh::client::should_resend_game_in_syn(false, false));
    EXPECT_FALSE(mxh::client::latch_game_in_syn(false));
    EXPECT_TRUE(mxh::client::latch_game_in_syn(true));
}

TEST(SessionRetryContract, InGameDisconnectRequestsTitleWithMessage9106) {
    const auto live = mxh::client::in_game_disconnect_notice(false, true, false);
    EXPECT_TRUE(live.notify);
    EXPECT_EQ(live.message_id, 9106);
    EXPECT_EQ(live.message_id, mxh::client::kDisconnectMessageId);
    EXPECT_EQ(live.next_state, mxh::client::kReloginStateId);
    EXPECT_EQ(live.next_state, 3);

    EXPECT_FALSE(mxh::client::in_game_disconnect_notice(true, true, false).notify);
    EXPECT_FALSE(mxh::client::in_game_disconnect_notice(false, false, false).notify);
    EXPECT_FALSE(mxh::client::in_game_disconnect_notice(false, true, true).notify);
}

TEST(SessionRetryContract, ReleaseAllowsSecondStartAndStartedGuardBlocksDoubleStart) {
    EXPECT_TRUE(mxh::client::ignore_duplicate_login_start(true));
    EXPECT_FALSE(mxh::client::ignore_duplicate_login_start(false));
}

TEST(SessionRetryContract, LoginFailureWipesPassword) {
    std::string secret = "s3cret-value";
    const auto size = secret.size();
    mxh::client::zero_secret(secret);
    ASSERT_EQ(secret.size(), size);
    EXPECT_EQ(secret, std::string(size, '\0'));
    mxh::client::wipe_secret(secret);
    EXPECT_TRUE(secret.empty());

    std::string again = "s3cret-value";
    mxh::client::wipe_secret(again);
    EXPECT_TRUE(again.empty());
    EXPECT_EQ(again.find("s3cret"), std::string::npos);
}
