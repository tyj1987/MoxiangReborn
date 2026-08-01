#include "mxh/server/common_network_msg_parser.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {
using mxh::server::CommonParserTables;
using mxh::server::mp_max;
using mxh::server::MessageParser;

struct ParserCounter { int calls = 0; std::uint32_t last_arg = 0; std::size_t last_size = 0; };

MessageParser make_counter(ParserCounter& c) {
    return [&c](std::uint32_t arg, const std::vector<std::uint8_t>& pl) {
        ++c.calls;
        c.last_arg = arg;
        c.last_size = pl.size();
    };
}

}  // namespace

TEST(CommonParserTables, AllocatesLegacyMpMaxSlots) {
    CommonParserTables t;
    EXPECT_EQ(t.server.size(), mp_max);
    EXPECT_EQ(t.user.size(), mp_max);
}

TEST(CommonParserTables, AllSlotsStartEmpty) {
    CommonParserTables t;
    for (std::size_t i = 0; i < mp_max; ++i) {
        EXPECT_FALSE(static_cast<bool>(t.server[i]));
        EXPECT_FALSE(static_cast<bool>(t.user[i]));
    }
}

TEST(CommonParserTables, EmptyServerSlotIsNoOp) {
    CommonParserTables t;
    EXPECT_TRUE(t.invoke_server(1, 2, {}));
    EXPECT_TRUE(t.invoke_server(mp_max - 1, 99, {}));
}

TEST(CommonParserTables, EmptyUserSlotIsNoOp) {
    CommonParserTables t;
    EXPECT_TRUE(t.invoke_user(1, 2, {}));
    EXPECT_TRUE(t.invoke_user(mp_max - 1, 0, {}));
}

TEST(CommonParserTables, OutOfRangeRejected) {
    CommonParserTables t;
    EXPECT_FALSE(t.invoke_user(mp_max, 0, {}));
    EXPECT_FALSE(t.invoke_server(mp_max, 0, {}));
    EXPECT_FALSE(t.invoke_server(mp_max + 1, 0, {}));
    EXPECT_FALSE(t.invoke_user(100000, 0, {}));
}

TEST(CommonParserTables, NullParserRejected) {
    CommonParserTables t;
    EXPECT_FALSE(t.set_server(2, {}));
    EXPECT_FALSE(t.set_user(2, {}));
}

TEST(CommonParserTables, SetReplacesExistingParserAtSameIdx) {
    CommonParserTables t;
    ParserCounter a, b;
    EXPECT_TRUE(t.set_server(3, make_counter(a)));
    t.invoke_server(3, 99, {});
    EXPECT_EQ(a.calls, 1);
    EXPECT_EQ(a.last_arg, 99u);
    EXPECT_TRUE(t.set_server(3, make_counter(b)));
    t.invoke_server(3, 100, {});
    EXPECT_EQ(a.calls, 1);
    EXPECT_EQ(b.calls, 1);
    EXPECT_EQ(b.last_arg, 100u);
}

TEST(CommonParserTables, SetOutOfRangeRejected) {
    CommonParserTables t;
    ParserCounter c;
    EXPECT_FALSE(t.set_server(mp_max, make_counter(c)));
    EXPECT_FALSE(t.set_user(mp_max, make_counter(c)));
    EXPECT_FALSE(t.set_server(mp_max + 100, make_counter(c)));
}

TEST(CommonParserTables, DispatchesServerAndUserSeparately) {
    CommonParserTables t;
    ParserCounter a, b;
    EXPECT_TRUE(t.set_server(3, make_counter(a)));
    EXPECT_TRUE(t.set_user(3, make_counter(b)));
    EXPECT_TRUE(t.invoke_server(3, 0, {}));
    EXPECT_TRUE(t.invoke_user(3, 0, {}));
    EXPECT_EQ(a.calls, 1);
    EXPECT_EQ(b.calls, 1);
}

TEST(CommonParserTables, UserInvokeDoesNotFireServerHandler) {
    CommonParserTables t;
    ParserCounter srv, usr;
    EXPECT_TRUE(t.set_server(7, make_counter(srv)));
    EXPECT_TRUE(t.set_user(7, make_counter(usr)));
    EXPECT_TRUE(t.invoke_user(7, 0, {}));
    EXPECT_EQ(srv.calls, 0);
    EXPECT_EQ(usr.calls, 1);
}

TEST(CommonParserTables, DifferentIdxDoesNotCrossFire) {
    CommonParserTables t;
    ParserCounter two, three;
    EXPECT_TRUE(t.set_server(2, make_counter(two)));
    EXPECT_TRUE(t.set_server(3, make_counter(three)));
    EXPECT_TRUE(t.invoke_server(2, 0, {}));
    EXPECT_EQ(two.calls, 1);
    EXPECT_EQ(three.calls, 0);
    EXPECT_TRUE(t.invoke_server(3, 0, {}));
    EXPECT_EQ(three.calls, 1);
    EXPECT_EQ(two.calls, 1);
}

TEST(CommonParserTables, InvokePassesPayloadAndArgToHandler) {
    CommonParserTables t;
    ParserCounter srv;
    EXPECT_TRUE(t.set_server(5, make_counter(srv)));
    const std::vector<std::uint8_t> pl{0xAAu, 0xBBu, 0xCCu, 0xDDu};
    EXPECT_TRUE(t.invoke_server(5, 4242u, pl));
    EXPECT_EQ(srv.calls, 1);
    EXPECT_EQ(srv.last_arg, 4242u);
    EXPECT_EQ(srv.last_size, pl.size());
}

TEST(CommonParserTables, MultipleInvokesAccumulateCount) {
    CommonParserTables t;
    ParserCounter c;
    EXPECT_TRUE(t.set_server(0, make_counter(c)));
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(t.invoke_server(0, static_cast<std::uint32_t>(i), {}));
    }
    EXPECT_EQ(c.calls, 100);
    EXPECT_EQ(c.last_arg, 99u);
}

TEST(CommonParserTables, MpMaxIs96ForLegacyProtocolTable) {
    EXPECT_EQ(mp_max, 96u);
}

TEST(CommonParserTables, FillAllMaxSlotsAndVerifyEachFiresIndependently) {
    std::vector<ParserCounter> counters(mp_max);
    CommonParserTables t;
    for (std::size_t i = 0; i < mp_max; ++i) {
        EXPECT_TRUE(t.set_server(i, make_counter(counters[i])));
        EXPECT_EQ(counters[i].calls, 0);
    }
    for (std::size_t i = 0; i < mp_max; ++i) {
        EXPECT_TRUE(t.invoke_server(i, static_cast<std::uint32_t>(i), {}));
    }
    for (std::size_t i = 0; i < mp_max; ++i) {
        EXPECT_EQ(counters[i].calls, 1);
        EXPECT_EQ(counters[i].last_arg, static_cast<std::uint32_t>(i));
    }
}

TEST(CommonParserTables, ServerAndUserAtSameIdxAreIndependent) {
    CommonParserTables t;
    ParserCounter srv, usr;
    EXPECT_TRUE(t.set_server(10, make_counter(srv)));
    EXPECT_TRUE(t.set_user(10, make_counter(usr)));
    EXPECT_TRUE(t.invoke_server(10, 0, {}));
    EXPECT_TRUE(t.invoke_user(10, 0, {}));
    EXPECT_EQ(srv.calls, 1);
    EXPECT_EQ(usr.calls, 1);
    EXPECT_FALSE(t.set_server(10, {}));
    EXPECT_TRUE(t.invoke_server(10, 0, {}));
    EXPECT_EQ(srv.calls, 2);
    EXPECT_EQ(usr.calls, 1);
}
