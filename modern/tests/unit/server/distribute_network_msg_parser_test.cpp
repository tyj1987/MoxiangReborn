// distribute_network_msg_parser_test.cpp

#include "mxh/server/distribute_network_msg_parser.hpp"
#include <gtest/gtest.h>
#include <atomic>

namespace {
using mxh::server::DistributeNetworkMsgParser;
using mxh::server::DispatchStatus;
}

TEST(DistributeNetworkMsgParser, RegisterAndDispatch) {
    DistributeNetworkMsgParser p;
    std::atomic<int> n{0};
    EXPECT_TRUE(p.register_handler(/*cat*/7, /*proto*/1, [&](auto,auto,auto,auto,auto){
        ++n;
        return DispatchStatus::Handled;
    }));
    EXPECT_EQ(p.handler_count(), 1u);
    EXPECT_EQ(p.dispatch(100, 7, 1, 7, {}), DispatchStatus::Handled);
    EXPECT_EQ(n.load(), 1);
}

TEST(DistributeNetworkMsgParser, UnknownProtocol) {
    DistributeNetworkMsgParser p;
    p.register_handler(7, 1, [](auto...){ return DispatchStatus::Handled; });
    EXPECT_EQ(p.dispatch(100, 7, /*proto*/999, 7, {}),
              DispatchStatus::UnknownProtocol);
}

TEST(DistributeNetworkMsgParser, DistinctCategories) {
    DistributeNetworkMsgParser p;
    p.register_handler(7, 1, [](auto...){ return DispatchStatus::Handled; });
    p.register_handler(7, 2, [](auto...){ return DispatchStatus::Handled; });
    p.register_handler(8, 1, [](auto...){ return DispatchStatus::Handled; });
    EXPECT_EQ(p.handler_count(), 3u);
    EXPECT_EQ(p.count_for_category(7), 2u);
    EXPECT_EQ(p.count_for_category(8), 1u);
    EXPECT_EQ(p.count_for_category(9), 0u);
}

TEST(DistributeNetworkMsgParser, HandlerCanReturnInvalidSession) {
    DistributeNetworkMsgParser p;
    p.register_handler(7, 1, [](std::uint32_t sid, auto, auto, auto, auto){
        return sid == 0 ? DispatchStatus::InvalidSession : DispatchStatus::Handled;
    });
    EXPECT_EQ(p.dispatch(0,   7, 1, 7, {}), DispatchStatus::InvalidSession);
    EXPECT_EQ(p.dispatch(123, 7, 1, 7, {}), DispatchStatus::Handled);
}

TEST(DistributeNetworkMsgParser, RegisterRejectsNull) {
    DistributeNetworkMsgParser p;
    EXPECT_FALSE(p.register_handler(7, 1, {}));
    EXPECT_EQ(p.handler_count(), 0u);
}
TEST(DistributeNetworkMsgParser, RegisterOverridesSameKey) {
    DistributeNetworkMsgParser p;
    int a = 0, b = 0;
    EXPECT_TRUE(p.register_handler(7, 1, [&](auto,auto,auto,auto,auto){ ++a; return DispatchStatus::Handled; }));
    EXPECT_TRUE(p.register_handler(7, 1, [&](auto,auto,auto,auto,auto){ ++b; return DispatchStatus::Handled; }));
    EXPECT_EQ(p.handler_count(), 1u);  // override
    EXPECT_EQ(p.dispatch(0, 7, 1, 0, {}), DispatchStatus::Handled);
    EXPECT_EQ(a, 0);
    EXPECT_EQ(b, 1);  // only the latest one fires
}

TEST(DistributeNetworkMsgParser, DispatchDoesNotInvokeOtherHandlers) {
    DistributeNetworkMsgParser p;
    int seven_one = 0, seven_two = 0, eight_one = 0;
    p.register_handler(7, 1, [&](auto,auto,auto,auto,auto){ ++seven_one; return DispatchStatus::Handled; });
    p.register_handler(7, 2, [&](auto,auto,auto,auto,auto){ ++seven_two; return DispatchStatus::Handled; });
    p.register_handler(8, 1, [&](auto,auto,auto,auto,auto){ ++eight_one; return DispatchStatus::Handled; });
    EXPECT_EQ(p.dispatch(1, 7, 1, 0, {}), DispatchStatus::Handled);
    EXPECT_EQ(seven_one, 1);
    EXPECT_EQ(seven_two, 0);
    EXPECT_EQ(eight_one, 0);
    EXPECT_EQ(p.dispatch(1, 8, 1, 0, {}), DispatchStatus::Handled);
    EXPECT_EQ(eight_one, 1);
    EXPECT_EQ(seven_one, 1);
}

TEST(DistributeNetworkMsgParser, HandlerReceivesExactArguments) {
    DistributeNetworkMsgParser p;
    std::uint32_t got_sid = 0, got_oid = 0, got_cat = 999;
    std::uint16_t got_proto = 0;
    std::vector<std::uint8_t> got_payload;
    p.register_handler(7, 42, [&](std::uint32_t sid, std::uint16_t proto, std::uint32_t oid, std::uint32_t cat, const std::vector<std::uint8_t>& pl){
        got_sid = sid;
        got_proto = proto;
        got_oid = oid;
        got_cat = cat;
        got_payload = pl;
        return DispatchStatus::Handled;
    });
    const std::vector<std::uint8_t> payload{0xAAu, 0xBBu, 0xCCu, 0xDDu};
    EXPECT_EQ(p.dispatch(12345u, 7u, 42u, 99u, payload), DispatchStatus::Handled);
    EXPECT_EQ(got_sid, 12345u);
    EXPECT_EQ(got_proto, 42u);
    EXPECT_EQ(got_oid, 99u);
    EXPECT_EQ(got_cat, 7u);
    EXPECT_EQ(got_payload, payload);
}

TEST(DistributeNetworkMsgParser, UnknownCategoryIsNotHandled) {
    DistributeNetworkMsgParser p;
    p.register_handler(7, 1, [](auto...){ return DispatchStatus::Handled; });
    EXPECT_EQ(p.dispatch(1, 250u, 1u, 0, {}), DispatchStatus::UnknownProtocol);
}

TEST(DistributeNetworkMsgParser, EmptyPayloadStillInvokesHandler) {
    DistributeNetworkMsgParser p;
    int called = 0;
    p.register_handler(7, 1, [&](auto,auto,auto,auto,const std::vector<std::uint8_t>& pl){
        if (pl.empty()) ++called;
        return DispatchStatus::Handled;
    });
    EXPECT_EQ(p.dispatch(1, 7, 1, 0, {}), DispatchStatus::Handled);
    EXPECT_EQ(called, 1);
}

TEST(DistributeNetworkMsgParser, MaxProtocolValueIsStored) {
    DistributeNetworkMsgParser p;
    int called = 0;
    p.register_handler(7, 65535u, [&](auto,auto,auto,auto,auto){ ++called; return DispatchStatus::Handled; });
    EXPECT_EQ(p.dispatch(1, 7, 65535u, 0, {}), DispatchStatus::Handled);
    EXPECT_EQ(called, 1);
}

TEST(DistributeNetworkMsgParser, MaxCategoryValueIsStored) {
    DistributeNetworkMsgParser p;
    int called = 0;
    p.register_handler(255u, 1, [&](auto,auto,auto,auto,auto){ ++called; return DispatchStatus::Handled; });
    EXPECT_EQ(p.dispatch(1, 255u, 1, 0, {}), DispatchStatus::Handled);
    EXPECT_EQ(called, 1);
    EXPECT_EQ(p.count_for_category(255u), 1u);
}

TEST(DistributeNetworkMsgParser, CountForCategoryIsZeroByDefault) {
    DistributeNetworkMsgParser p;
    EXPECT_EQ(p.count_for_category(0u), 0u);
    EXPECT_EQ(p.count_for_category(7u), 0u);
    EXPECT_EQ(p.count_for_category(255u), 0u);
}
