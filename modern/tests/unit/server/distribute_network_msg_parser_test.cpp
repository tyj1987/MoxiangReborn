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
