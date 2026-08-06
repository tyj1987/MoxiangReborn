// agent_mugong_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::classify_mugong (D4.145).
// Augments the legacy 11-test agent_mugong_test.cpp with deeper coverage of:
//   - mugong_category constant = 9 (MP_MUGONG)
//   - 25 sub-protocol constants (mugong_totalinfo_local=0 .. mugong_option_clear_nack=24)
//   - MugongActionKind enum (forward_to_map, forward_to_map_if_level_ok, send_nack)
//   - MugongRequest struct defaults (protocol=0, object_id=0, mugong_index=0,
//     mugong_level=0, required_level=0)
//   - MugongAction struct defaults
//   - classify_mugong truth table:
//       protocol=mugong_option_syn + required_level>0 + mugong_level<required_level -> send_nack
//       otherwise -> forward_to_map with original protocol + error_code=0
//   - boundary tests: required_level==0 disables gate, mugong_level==required_level accepted
//
// 1:1 invariants (locked):
//   - mugong_category = 9
//   - 25 protocol constants (0..24)
//   - Level gate fires iff all 3: protocol==mugong_option_syn, required_level>0,
//     mugong_level<required_level
//   - send_nack uses protocol=mugong_option_nack + error_code=1
//   - Forward preserves input protocol + zero error_code

#pragma once

#include "mxh/server/agent_mugong.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <type_traits>

namespace {

using mxh::server::classify_mugong;
using mxh::server::mugong_add_ack;
using mxh::server::mugong_add_nack;
using mxh::server::mugong_add_syn;
using mxh::server::mugong_category;
using mxh::server::mugong_deletegroundadd_ack;
using mxh::server::mugong_deletegroundadd_nack;
using mxh::server::mugong_deletegroundadd_syn;
using mxh::server::mugong_deleteinventoryadd_ack;
using mxh::server::mugong_deleteinventoryadd_nack;
using mxh::server::mugong_deleteinventoryadd_syn;
using mxh::server::mugong_exppoint_notify;
using mxh::server::mugong_move_ack;
using mxh::server::mugong_move_nack;
using mxh::server::mugong_move_syn;
using mxh::server::mugong_option_ack;
using mxh::server::mugong_option_clear_ack;
using mxh::server::mugong_option_clear_nack;
using mxh::server::mugong_option_clear_syn;
using mxh::server::mugong_option_nack;
using mxh::server::mugong_option_syn;
using mxh::server::mugong_rem_ack;
using mxh::server::mugong_rem_nack;
using mxh::server::mugong_rem_syn;
using mxh::server::mugong_sung_levelup;
using mxh::server::mugong_sung_notify;
using mxh::server::mugong_totalinfo_local;
using mxh::server::MugongAction;
using mxh::server::MugongActionKind;
using mxh::server::MugongRequest;

}  // namespace


// ===========================================================================
// mugong_category constant
// ===========================================================================

TEST(AgentMugongDataPlane, CategoryIsNine) {
    EXPECT_EQ(mugong_category, 9u);
}


// ===========================================================================
// Protocol constants
// ===========================================================================

TEST(AgentMugongDataPlane, ProtocolTotalinfoLocalIsZero) { EXPECT_EQ(mugong_totalinfo_local, 0u); }
TEST(AgentMugongDataPlane, ProtocolMoveSynIsOne) { EXPECT_EQ(mugong_move_syn, 1u); }
TEST(AgentMugongDataPlane, ProtocolMoveAckIsTwo) { EXPECT_EQ(mugong_move_ack, 2u); }
TEST(AgentMugongDataPlane, ProtocolMoveNackIsThree) { EXPECT_EQ(mugong_move_nack, 3u); }
TEST(AgentMugongDataPlane, ProtocolRemSynIsFour) { EXPECT_EQ(mugong_rem_syn, 4u); }
TEST(AgentMugongDataPlane, ProtocolRemAckIsFive) { EXPECT_EQ(mugong_rem_ack, 5u); }
TEST(AgentMugongDataPlane, ProtocolRemNackIsSix) { EXPECT_EQ(mugong_rem_nack, 6u); }
TEST(AgentMugongDataPlane, ProtocolAddSynIsSeven) { EXPECT_EQ(mugong_add_syn, 7u); }
TEST(AgentMugongDataPlane, ProtocolAddAckIsEight) { EXPECT_EQ(mugong_add_ack, 8u); }
TEST(AgentMugongDataPlane, ProtocolAddNackIsNine) { EXPECT_EQ(mugong_add_nack, 9u); }
TEST(AgentMugongDataPlane, ProtocolDeletegroundaddSynIsTen) { EXPECT_EQ(mugong_deletegroundadd_syn, 10u); }
TEST(AgentMugongDataPlane, ProtocolDeletegroundaddAckIsEleven) { EXPECT_EQ(mugong_deletegroundadd_ack, 11u); }
TEST(AgentMugongDataPlane, ProtocolDeletegroundaddNackIsTwelve) { EXPECT_EQ(mugong_deletegroundadd_nack, 12u); }
TEST(AgentMugongDataPlane, ProtocolDeleteinventoryaddSynIsThirteen) { EXPECT_EQ(mugong_deleteinventoryadd_syn, 13u); }
TEST(AgentMugongDataPlane, ProtocolDeleteinventoryaddAckIsFourteen) { EXPECT_EQ(mugong_deleteinventoryadd_ack, 14u); }
TEST(AgentMugongDataPlane, ProtocolDeleteinventoryaddNackIsFifteen) { EXPECT_EQ(mugong_deleteinventoryadd_nack, 15u); }
TEST(AgentMugongDataPlane, ProtocolExppointNotifyIsSixteen) { EXPECT_EQ(mugong_exppoint_notify, 16u); }
TEST(AgentMugongDataPlane, ProtocolSungNotifyIsSeventeen) { EXPECT_EQ(mugong_sung_notify, 17u); }
TEST(AgentMugongDataPlane, ProtocolSungLevelupIsEighteen) { EXPECT_EQ(mugong_sung_levelup, 18u); }
TEST(AgentMugongDataPlane, ProtocolOptionSynIsNineteen) { EXPECT_EQ(mugong_option_syn, 19u); }
TEST(AgentMugongDataPlane, ProtocolOptionAckIsTwenty) { EXPECT_EQ(mugong_option_ack, 20u); }
TEST(AgentMugongDataPlane, ProtocolOptionNackIsTwentyOne) { EXPECT_EQ(mugong_option_nack, 21u); }
TEST(AgentMugongDataPlane, ProtocolOptionClearSynIsTwentyTwo) { EXPECT_EQ(mugong_option_clear_syn, 22u); }
TEST(AgentMugongDataPlane, ProtocolOptionClearAckIsTwentyThree) { EXPECT_EQ(mugong_option_clear_ack, 23u); }
TEST(AgentMugongDataPlane, ProtocolOptionClearNackIsTwentyFour) { EXPECT_EQ(mugong_option_clear_nack, 24u); }

TEST(AgentMugongDataPlane, ProtocolConstantsAllDistinct) {
    std::set<std::uint8_t> seen = {
        mugong_totalinfo_local, mugong_move_syn, mugong_move_ack, mugong_move_nack,
        mugong_rem_syn, mugong_rem_ack, mugong_rem_nack,
        mugong_add_syn, mugong_add_ack, mugong_add_nack,
        mugong_deletegroundadd_syn, mugong_deletegroundadd_ack, mugong_deletegroundadd_nack,
        mugong_deleteinventoryadd_syn, mugong_deleteinventoryadd_ack, mugong_deleteinventoryadd_nack,
        mugong_exppoint_notify, mugong_sung_notify, mugong_sung_levelup,
        mugong_option_syn, mugong_option_ack, mugong_option_nack,
        mugong_option_clear_syn, mugong_option_clear_ack, mugong_option_clear_nack,
    };
    EXPECT_EQ(seen.size(), 25u);
}


// ===========================================================================
// Enum types
// ===========================================================================

TEST(AgentMugongDataPlane, ActionKindHasThreeValues) {
    auto all = {
        MugongActionKind::forward_to_map,
        MugongActionKind::forward_to_map_if_level_ok,
        MugongActionKind::send_nack,
    };
    EXPECT_EQ(all.size(), 3u);
}


// ===========================================================================
// Struct defaults
// ===========================================================================

TEST(AgentMugongDataPlane, RequestDefaults) {
    MugongRequest r{};
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.object_id, 0u);
    EXPECT_EQ(r.mugong_index, 0u);
    EXPECT_EQ(r.mugong_level, 0u);
    EXPECT_EQ(r.required_level, 0u);
}

TEST(AgentMugongDataPlane, ActionDefaults) {
    MugongAction a{};
    EXPECT_EQ(a.kind, MugongActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, 0u);
    EXPECT_EQ(a.object_id, 0u);
    EXPECT_EQ(a.error_code, 0u);
}


// ===========================================================================
// classify_mugong -- non-option_syn path (always forward)
// ===========================================================================

TEST(AgentMugongDataPlane, ClassifyMoveSynForwards) {
    MugongRequest r{mugong_move_syn, 5u, 0u, 0u, 0u};
    auto a = classify_mugong(r);
    EXPECT_EQ(a.kind, MugongActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, mugong_move_syn);
    EXPECT_EQ(a.object_id, 5u);
    EXPECT_EQ(a.error_code, 0u);
}

TEST(AgentMugongDataPlane, ClassifyAddSynForwards) {
    MugongRequest r{mugong_add_syn, 7u, 1u, 5u, 10u};
    auto a = classify_mugong(r);
    EXPECT_EQ(a.kind, MugongActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, mugong_add_syn);
}

TEST(AgentMugongDataPlane, ClassifyExppointNotifyForwards) {
    MugongRequest r{mugong_exppoint_notify, 7u, 0u, 0u, 0u};
    auto a = classify_mugong(r);
    EXPECT_EQ(a.kind, MugongActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, mugong_exppoint_notify);
}

TEST(AgentMugongDataPlane, ClassifySungLevelupForwards) {
    MugongRequest r{mugong_sung_levelup, 7u, 0u, 0u, 0u};
    auto a = classify_mugong(r);
    EXPECT_EQ(a.kind, MugongActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, mugong_sung_levelup);
}

TEST(AgentMugongDataPlane, ClassifyAllNonOptionSynProtocolsForward) {
    // All 24 non-option_syn protocols: all forward.
    std::uint8_t protos[] = {
        mugong_totalinfo_local, mugong_move_syn, mugong_move_ack, mugong_move_nack,
        mugong_rem_syn, mugong_rem_ack, mugong_rem_nack,
        mugong_add_syn, mugong_add_ack, mugong_add_nack,
        mugong_deletegroundadd_syn, mugong_deletegroundadd_ack, mugong_deletegroundadd_nack,
        mugong_deleteinventoryadd_syn, mugong_deleteinventoryadd_ack, mugong_deleteinventoryadd_nack,
        mugong_exppoint_notify, mugong_sung_notify, mugong_sung_levelup,
        mugong_option_ack, mugong_option_nack,
        mugong_option_clear_syn, mugong_option_clear_ack, mugong_option_clear_nack,
    };
    for (auto p : protos) {
        MugongRequest r{p, 100u, 0u, 1u, 100u};  // level gate would fire if p==option_syn
        auto a = classify_mugong(r);
        EXPECT_EQ(a.kind, MugongActionKind::forward_to_map);
        EXPECT_EQ(a.protocol, p);
        EXPECT_EQ(a.error_code, 0u);
    }
}


// ===========================================================================
// classify_mugong -- option_syn level gate
// ===========================================================================

TEST(AgentMugongDataPlane, ClassifyOptionSynBelowRequiredRejects) {
    MugongRequest r{mugong_option_syn, 5u, 1u, 5u, 10u};
    auto a = classify_mugong(r);
    EXPECT_EQ(a.kind, MugongActionKind::send_nack);
    EXPECT_EQ(a.protocol, mugong_option_nack);
    EXPECT_EQ(a.error_code, 1u);
}

TEST(AgentMugongDataPlane, ClassifyOptionSynAtRequiredAccepts) {
    MugongRequest r{mugong_option_syn, 5u, 1u, 10u, 10u};
    auto a = classify_mugong(r);
    EXPECT_EQ(a.kind, MugongActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, mugong_option_syn);
    EXPECT_EQ(a.error_code, 0u);
}

TEST(AgentMugongDataPlane, ClassifyOptionSynAboveRequiredAccepts) {
    MugongRequest r{mugong_option_syn, 5u, 1u, 15u, 10u};
    auto a = classify_mugong(r);
    EXPECT_EQ(a.kind, MugongActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, mugong_option_syn);
}

TEST(AgentMugongDataPlane, ClassifyOptionSynZeroRequiredLevelAccepts) {
    // required_level=0 disables the gate.
    MugongRequest r{mugong_option_syn, 5u, 1u, 1u, 0u};
    auto a = classify_mugong(r);
    EXPECT_EQ(a.kind, MugongActionKind::forward_to_map);
}

TEST(AgentMugongDataPlane, ClassifyOptionSynZeroLevelNonZeroRequiredRejects) {
    MugongRequest r{mugong_option_syn, 5u, 1u, 0u, 1u};
    auto a = classify_mugong(r);
    EXPECT_EQ(a.kind, MugongActionKind::send_nack);
    EXPECT_EQ(a.error_code, 1u);
}

TEST(AgentMugongDataPlane, ClassifyOptionSynMaxLevelRejects) {
    // mugong_level=0 required_level=max -> reject
    MugongRequest r{mugong_option_syn, 5u, 1u, 0u, 0xFFFFFFFFu};
    auto a = classify_mugong(r);
    EXPECT_EQ(a.kind, MugongActionKind::send_nack);
}

TEST(AgentMugongDataPlane, ClassifyOptionSynMaxLevelMaxRequiredAccepts) {
    MugongRequest r{mugong_option_syn, 5u, 1u, 0xFFFFFFFFu, 0xFFFFFFFFu};
    auto a = classify_mugong(r);
    EXPECT_EQ(a.kind, MugongActionKind::forward_to_map);
}


// ===========================================================================
// Nack path preservation
// ===========================================================================

TEST(AgentMugongDataPlane, ClassifyOptionSynNackPreservesObjectId) {
    MugongRequest r{mugong_option_syn, 0xDEADBEEFu, 1u, 1u, 100u};
    auto a = classify_mugong(r);
    EXPECT_EQ(a.object_id, 0xDEADBEEFu);
}

TEST(AgentMugongDataPlane, ClassifyOptionSynNackIgnoresMugongIndex) {
    MugongRequest r{mugong_option_syn, 5u, 999u, 1u, 100u};
    auto a = classify_mugong(r);
    EXPECT_EQ(a.kind, MugongActionKind::send_nack);
    // mugong_index doesn't appear in action; it's only a request field.
}


// ===========================================================================
// Forward path preservation
// ===========================================================================

TEST(AgentMugongDataPlane, ClassifyForwardPreservesObjectIdMaxUint32) {
    MugongRequest r{mugong_move_syn, 0xFFFFFFFFu, 0u, 0u, 0u};
    auto a = classify_mugong(r);
    EXPECT_EQ(a.object_id, 0xFFFFFFFFu);
}

TEST(AgentMugongDataPlane, ClassifyForwardZeroErrorCode) {
    MugongRequest r{mugong_move_syn, 5u, 0u, 0u, 0u};
    auto a = classify_mugong(r);
    EXPECT_EQ(a.error_code, 0u);
}

TEST(AgentMugongDataPlane, ClassifyDefaultForwards) {
    // Default-constructed request: protocol=0 (mugong_totalinfo_local), all zero -> forward.
    MugongRequest r{};
    auto a = classify_mugong(r);
    EXPECT_EQ(a.kind, MugongActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, 0u);
}
