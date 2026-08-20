#include "CEngine.hpp"

#include <gtest/gtest.h>

namespace {

TEST(StateTransfer, ConsumesTypedPayloadExactlyOnce) {
    mxh::client::CEngine engine;
    mxh::client::LoginResult login;
    login.agent_addr = "192.168.2.117";
    login.agent_port = 17001;
    login.user_idx = 42;
    engine.SetPendingTransfer(std::move(login));

    auto first = engine.TakePendingTransfer();
    const auto* consumed = std::get_if<mxh::client::LoginResult>(&first);
    ASSERT_NE(consumed, nullptr);
    EXPECT_EQ(consumed->agent_addr, "192.168.2.117");
    EXPECT_EQ(consumed->user_idx, 42u);

    auto second = engine.TakePendingTransfer();
    EXPECT_TRUE(std::holds_alternative<std::monostate>(second));
}

TEST(StateTransfer, DoesNotConfuseGameEntryWithLoginResult) {
    mxh::client::CEngine engine;
    engine.SetPendingTransfer(mxh::client::GameEntryRequest{100000u, 12u});
    const auto transfer = engine.TakePendingTransfer();
    EXPECT_EQ(std::get<mxh::client::GameEntryRequest>(transfer).map_num, 12u);
    EXPECT_EQ(std::get_if<mxh::client::LoginResult>(&transfer), nullptr);
}

TEST(StateTransfer, PendingTypeCanBeInspectedWithoutConsumption) {
    mxh::client::CEngine engine;
    mxh::client::LoginResult login;
    login.agent_addr = "192.168.2.117";
    login.agent_port = 17001;
    engine.SetPendingTransfer(login);

    EXPECT_TRUE(engine.pending_transfer_is<mxh::client::LoginResult>());
    EXPECT_FALSE(engine.pending_transfer_is<mxh::client::GameEntryRequest>());
    EXPECT_TRUE(engine.has_pending_transfer());

    const auto transfer = engine.TakePendingTransfer();
    ASSERT_TRUE(std::holds_alternative<mxh::client::LoginResult>(transfer));
    EXPECT_FALSE(engine.has_pending_transfer());
}

}  // namespace
