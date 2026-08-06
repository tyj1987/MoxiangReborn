// agent_db_msg_parser_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::AgentDbDispatcher (D4.139).
// Augments the legacy 6-test agent_db_msg_parser_test.cpp with deeper coverage of:
//   - max_query constant = 76
//   - AgentDbQuery enum (22 distinct values, 1..54 with gaps)
//   - AgentDbResult struct defaults (query_id=0, connection_index=0, result=0,
//     rows=empty nested vector)
//   - AgentDbHandler type alias (std::function<void(const AgentDbResult&)>)
//   - AgentDbDispatcher default state (handler_count=0)
//   - register_handler: valid slot (<76) registers; out-of-range (>=76) ignored
//   - register_handler: null handler ignored
//   - register_handler: duplicate replaces (count stays 1, last-wins)
//   - register_handler: many distinct query_ids all register
//   - dispatch: out-of-range query_id returns false (no invocation)
//   - dispatch: in-range but unregistered returns true (no-op)
//   - dispatch: invokes handler with full result struct
//   - has_slot: boundary values 0, 75 (true), 76, 65535 (false)
//   - rows: nested vector preserves multi-row multi-col data
//
// 1:1 invariants (locked):
//   - max_query = 76 (legacy DB dispatch table size)
//   - AgentDbQuery enum values are all distinct and in [1,54]
//   - AgentDbDispatcher::register_handler only writes when (id<max_query) AND handler
//   - AgentDbDispatcher::dispatch only invokes when (query_id<max_query) AND slot registered
//   - dispatch returns false iff query_id >= max_query

#pragma once

#include "mxh/server/agent_db_msg_parser.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {

using mxh::server::AgentDbDispatcher;
using mxh::server::AgentDbHandler;
using mxh::server::AgentDbQuery;
using mxh::server::AgentDbResult;
using mxh::server::max_query;

}  // namespace


// ===========================================================================
// max_query constant
// ===========================================================================

TEST(AgentDbDispatcherDataPlane, MaxQueryIsSeventySix) {
    EXPECT_EQ(max_query, 76u);
}

TEST(AgentDbDispatcherDataPlane, MaxQueryIsSizeT) {
    EXPECT_TRUE((std::is_same<decltype(max_query), const std::size_t>::value));
}


// ===========================================================================
// AgentDbQuery enum
// ===========================================================================

TEST(AgentDbDispatcherDataPlane, QueryEnumHasAtLeastTwentyEntries) {
    // Smoke-check: we know there are 22 named entries.
    auto names = {
        AgentDbQuery::character_base, AgentDbQuery::create_character,
        AgentDbQuery::login_check_delete, AgentDbQuery::delete_character,
        AgentDbQuery::name_check, AgentDbQuery::friend_add,
        AgentDbQuery::friend_list, AgentDbQuery::note_list,
        AgentDbQuery::wanted_delete, AgentDbQuery::gm_ban_character,
        AgentDbQuery::gm_update_user_level, AgentDbQuery::gm_where_is,
        AgentDbQuery::gm_login, AgentDbQuery::gm_power_list,
        AgentDbQuery::jackpot_total_money, AgentDbQuery::field_war_check_money,
        AgentDbQuery::field_war_add_money, AgentDbQuery::chase_find_user,
        AgentDbQuery::character_slot, AgentDbQuery::update_user_state,
        AgentDbQuery::punish_list_load, AgentDbQuery::punish_count_add,
    };
    EXPECT_EQ(names.size(), 22u);
}

TEST(AgentDbDispatcherDataPlane, QueryEnumUnderlyingTypeIsUint16) {
    EXPECT_TRUE((std::is_same<std::underlying_type_t<AgentDbQuery>, std::uint16_t>::value));
}

TEST(AgentDbDispatcherDataPlane, QueryEnumValuesAllDistinct) {
    auto names = {
        AgentDbQuery::character_base, AgentDbQuery::create_character,
        AgentDbQuery::login_check_delete, AgentDbQuery::delete_character,
        AgentDbQuery::name_check, AgentDbQuery::friend_add,
        AgentDbQuery::friend_list, AgentDbQuery::note_list,
        AgentDbQuery::wanted_delete, AgentDbQuery::gm_ban_character,
        AgentDbQuery::gm_update_user_level, AgentDbQuery::gm_where_is,
        AgentDbQuery::gm_login, AgentDbQuery::gm_power_list,
        AgentDbQuery::jackpot_total_money, AgentDbQuery::field_war_check_money,
        AgentDbQuery::field_war_add_money, AgentDbQuery::chase_find_user,
        AgentDbQuery::character_slot, AgentDbQuery::update_user_state,
        AgentDbQuery::punish_list_load, AgentDbQuery::punish_count_add,
    };
    std::vector<std::uint16_t> seen;
    for (auto q : names) {
        seen.push_back(static_cast<std::uint16_t>(q));
    }
    std::sort(seen.begin(), seen.end());
    EXPECT_EQ(std::adjacent_find(seen.begin(), seen.end()), seen.end());
}

TEST(AgentDbDispatcherDataPlane, QueryEnumValuesWithinBounds) {
    auto names = {
        AgentDbQuery::character_base, AgentDbQuery::create_character,
        AgentDbQuery::login_check_delete, AgentDbQuery::delete_character,
        AgentDbQuery::name_check, AgentDbQuery::friend_add,
        AgentDbQuery::friend_list, AgentDbQuery::note_list,
        AgentDbQuery::wanted_delete, AgentDbQuery::gm_ban_character,
        AgentDbQuery::gm_update_user_level, AgentDbQuery::gm_where_is,
        AgentDbQuery::gm_login, AgentDbQuery::gm_power_list,
        AgentDbQuery::jackpot_total_money, AgentDbQuery::field_war_check_money,
        AgentDbQuery::field_war_add_money, AgentDbQuery::chase_find_user,
        AgentDbQuery::character_slot, AgentDbQuery::update_user_state,
        AgentDbQuery::punish_list_load, AgentDbQuery::punish_count_add,
    };
    for (auto q : names) {
        auto v = static_cast<std::uint16_t>(q);
        EXPECT_LT(v, max_query);
        EXPECT_GE(v, 1u);
    }
}


// ===========================================================================
// AgentDbResult struct defaults
// ===========================================================================

TEST(AgentDbDispatcherDataPlane, ResultDefaultsQueryIdZero) {
    AgentDbResult r{};
    EXPECT_EQ(r.query_id, 0u);
}

TEST(AgentDbDispatcherDataPlane, ResultDefaultsConnectionIndexZero) {
    AgentDbResult r{};
    EXPECT_EQ(r.connection_index, 0u);
}

TEST(AgentDbDispatcherDataPlane, ResultDefaultsResultZero) {
    AgentDbResult r{};
    EXPECT_EQ(r.result, 0);
}

TEST(AgentDbDispatcherDataPlane, ResultDefaultsRowsEmpty) {
    AgentDbResult r{};
    EXPECT_TRUE(r.rows.empty());
}

TEST(AgentDbDispatcherDataPlane, ResultAcceptsAllFields) {
    AgentDbResult r;
    r.query_id = 5;
    r.connection_index = 42;
    r.result = -1;
    r.rows = {{"a", "b"}, {"c"}};
    EXPECT_EQ(r.query_id, 5u);
    EXPECT_EQ(r.connection_index, 42u);
    EXPECT_EQ(r.result, -1);
    ASSERT_EQ(r.rows.size(), 2u);
    EXPECT_EQ(r.rows[0].size(), 2u);
    EXPECT_EQ(r.rows[1].size(), 1u);
}


// ===========================================================================
// AgentDbDispatcher default state
// ===========================================================================

TEST(AgentDbDispatcherDataPlane, DefaultHandlerCountIsZero) {
    AgentDbDispatcher d;
    EXPECT_EQ(d.handler_count(), 0u);
}

TEST(AgentDbDispatcherDataPlane, DefaultDispatcherIsCopyable) {
    AgentDbDispatcher d;
    AgentDbDispatcher copy = d;
    EXPECT_EQ(copy.handler_count(), 0u);
}


// ===========================================================================
// register_handler
// ===========================================================================

TEST(AgentDbDispatcherDataPlane, RegisterHandlerIncrementsCount) {
    AgentDbDispatcher d;
    d.register_handler(1, [](const AgentDbResult&){});
    EXPECT_EQ(d.handler_count(), 1u);
}

TEST(AgentDbDispatcherDataPlane, RegisterHandlerMultipleSlotsAllRegistered) {
    AgentDbDispatcher d;
    for (std::uint16_t i = 0; i < 10; ++i) {
        d.register_handler(i, [](const AgentDbResult&){});
    }
    EXPECT_EQ(d.handler_count(), 10u);
}

TEST(AgentDbDispatcherDataPlane, RegisterHandlerDuplicateReplaces) {
    AgentDbDispatcher d;
    int first_called = 0, second_called = 0;
    d.register_handler(1, [&](const AgentDbResult&){ ++first_called; });
    d.register_handler(1, [&](const AgentDbResult&){ ++second_called; });
    EXPECT_EQ(d.handler_count(), 1u);
    AgentDbResult r;
    r.query_id = 1;
    d.dispatch(r);
    EXPECT_EQ(first_called, 0);
    EXPECT_EQ(second_called, 1);
}

TEST(AgentDbDispatcherDataPlane, RegisterHandlerBoundaryMaxQueryMinusOne) {
    AgentDbDispatcher d;
    d.register_handler(static_cast<std::uint16_t>(max_query - 1), [](const AgentDbResult&){});
    EXPECT_EQ(d.handler_count(), 1u);
}

TEST(AgentDbDispatcherDataPlane, RegisterHandlerRejectsMaxQuery) {
    AgentDbDispatcher d;
    d.register_handler(static_cast<std::uint16_t>(max_query), [](const AgentDbResult&){});
    EXPECT_EQ(d.handler_count(), 0u);
}

TEST(AgentDbDispatcherDataPlane, RegisterHandlerRejectsBeyondMaxQuery) {
    AgentDbDispatcher d;
    d.register_handler(static_cast<std::uint16_t>(max_query + 100), [](const AgentDbResult&){});
    EXPECT_EQ(d.handler_count(), 0u);
}

TEST(AgentDbDispatcherDataPlane, RegisterHandlerRejectsUint16Max) {
    AgentDbDispatcher d;
    d.register_handler(65535, [](const AgentDbResult&){});
    EXPECT_EQ(d.handler_count(), 0u);
}

TEST(AgentDbDispatcherDataPlane, RegisterHandlerNullHandlerIgnored) {
    AgentDbDispatcher d;
    d.register_handler(1, {});
    EXPECT_EQ(d.handler_count(), 0u);
}


// ===========================================================================
// dispatch
// ===========================================================================

TEST(AgentDbDispatcherDataPlane, DispatchInvokesRegisteredHandler) {
    AgentDbDispatcher d;
    int called = 0;
    d.register_handler(5, [&](const AgentDbResult& r) {
        ++called;
        EXPECT_EQ(r.query_id, 5u);
        EXPECT_EQ(r.result, 99);
    });
    AgentDbResult r;
    r.query_id = 5;
    r.result = 99;
    EXPECT_TRUE(d.dispatch(r));
    EXPECT_EQ(called, 1);
}

TEST(AgentDbDispatcherDataPlane, DispatchUnregisteredReturnsTrueNoOp) {
    AgentDbDispatcher d;
    EXPECT_TRUE(d.dispatch({}));  // empty result, query_id=0, no handler
}

TEST(AgentDbDispatcherDataPlane, DispatchOutOfRangeReturnsFalse) {
    AgentDbDispatcher d;
    AgentDbResult r;
    r.query_id = static_cast<std::uint16_t>(max_query);
    EXPECT_FALSE(d.dispatch(r));
}

TEST(AgentDbDispatcherDataPlane, DispatchOutOfRangeDoesNotInvoke) {
    AgentDbDispatcher d;
    int called = 0;
    d.register_handler(5, [&](const AgentDbResult&){ ++called; });
    AgentDbResult r;
    r.query_id = static_cast<std::uint16_t>(max_query + 10);
    EXPECT_FALSE(d.dispatch(r));
    EXPECT_EQ(called, 0);
}

TEST(AgentDbDispatcherDataPlane, DispatchPreservesConnectionIndex) {
    AgentDbDispatcher d;
    std::uint32_t observed = 0;
    d.register_handler(3, [&](const AgentDbResult& r) {
        observed = r.connection_index;
    });
    AgentDbResult r;
    r.query_id = 3;
    r.connection_index = 12345;
    d.dispatch(r);
    EXPECT_EQ(observed, 12345u);
}

TEST(AgentDbDispatcherDataPlane, DispatchPreservesRowsVector) {
    AgentDbDispatcher d;
    std::vector<std::vector<std::string_view>> observed;
    d.register_handler(7, [&](const AgentDbResult& r) {
        observed = r.rows;
    });
    AgentDbResult r;
    r.query_id = 7;
    r.rows = {{"alpha", "beta"}, {"gamma"}};
    d.dispatch(r);
    ASSERT_EQ(observed.size(), 2u);
    EXPECT_EQ(observed[0][0], "alpha");
    EXPECT_EQ(observed[1][0], "gamma");
}

TEST(AgentDbDispatcherDataPlane, DispatchMultipleSlotsAllInvocable) {
    AgentDbDispatcher d;
    int c1 = 0, c2 = 0, c3 = 0;
    d.register_handler(1, [&](const AgentDbResult&){ ++c1; });
    d.register_handler(2, [&](const AgentDbResult&){ ++c2; });
    d.register_handler(3, [&](const AgentDbResult&){ ++c3; });
    AgentDbResult r1; r1.query_id = 1;
    AgentDbResult r2; r2.query_id = 2;
    AgentDbResult r3; r3.query_id = 3;
    d.dispatch(r1);
    d.dispatch(r2);
    d.dispatch(r3);
    EXPECT_EQ(c1, 1);
    EXPECT_EQ(c2, 1);
    EXPECT_EQ(c3, 1);
}


// ===========================================================================
// has_slot
// ===========================================================================

TEST(AgentDbDispatcherDataPlane, HasSlotZero) {
    AgentDbDispatcher d;
    EXPECT_TRUE(d.has_slot(0));
}

TEST(AgentDbDispatcherDataPlane, HasSlotMaxQueryMinusOne) {
    AgentDbDispatcher d;
    EXPECT_TRUE(d.has_slot(static_cast<std::uint16_t>(max_query - 1)));
}

TEST(AgentDbDispatcherDataPlane, HasSlotMaxQuery) {
    AgentDbDispatcher d;
    EXPECT_FALSE(d.has_slot(static_cast<std::uint16_t>(max_query)));
}

TEST(AgentDbDispatcherDataPlane, HasSlotUint16Max) {
    AgentDbDispatcher d;
    EXPECT_FALSE(d.has_slot(65535));
}


// ===========================================================================
// Lifecycle
// ===========================================================================

TEST(AgentDbDispatcherDataPlane, DispatchAcrossDispatcherCopies) {
    AgentDbDispatcher a;
    int called = 0;
    a.register_handler(4, [&](const AgentDbResult&){ ++called; });
    AgentDbDispatcher b = a;  // copy ctor: handlers copied
    AgentDbResult r; r.query_id = 4;
    EXPECT_TRUE(b.dispatch(r));
    EXPECT_EQ(called, 1);
}

TEST(AgentDbDispatcherDataPlane, ManyHandlersLifecycle) {
    AgentDbDispatcher d;
    int total = 0;
    for (std::uint16_t i = 0; i < static_cast<std::uint16_t>(max_query); ++i) {
        d.register_handler(i, [&total](const AgentDbResult&){ ++total; });
    }
    EXPECT_EQ(d.handler_count(), max_query);
    for (std::uint16_t i = 0; i < static_cast<std::uint16_t>(max_query); ++i) {
        AgentDbResult r; r.query_id = i;
        d.dispatch(r);
    }
    EXPECT_EQ(total, static_cast<int>(max_query));
}
